#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <numeric>
#include <ostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;

namespace triewise {

// Basic limits used by the dictionary and search functions.
constexpr size_t max_word_length = 64;
constexpr uint64_t max_frequency = 1000000000000ULL;
constexpr uint32_t absent = numeric_limits<uint32_t>::max();

// Convert a word to lowercase and check that it only contains a-z.
inline string normalize(const string& text, bool allow_empty = false) {
    if (text.empty() && !allow_empty) {
        throw invalid_argument("Enter a non-empty word.");
    }

    if (text.size() > max_word_length) {
        throw invalid_argument("Maximum word length is 64 letters.");
    }

    string result;

    for (unsigned char ch : text) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<unsigned char>(ch - 'A' + 'a');
        }

        if (ch < 'a' || ch > 'z') {
            throw invalid_argument("Use English letters a-z only.");
        }

        result.push_back(static_cast<char>(ch));
    }

    return result;
}

// Convert a string to a number and check its allowed range.
inline uint64_t parse_integer(const string& text, uint64_t low, uint64_t high) {
    if (text.empty() || text.find_first_not_of("0123456789") != string::npos) {
        throw invalid_argument("Expected a non-negative integer.");
    }

    uint64_t value;

    try {
        value = stoull(text);
    }
    catch (const exception&) {
        throw invalid_argument("Integer is too large.");
    }

    if (value < low || value > high) {
        throw invalid_argument("Integer is outside the allowed range.");
    }

    return value;
}

// Each suggestion stores a word, its frequency and its edit distance.
struct Suggestion {
    string word;
    uint64_t frequency;
    int distance = 0;

    bool operator==(const Suggestion& other) const {
        return word == other.word &&
               frequency == other.frequency &&
               distance == other.distance;
    }
};

// This comparator puts better suggestions before weaker suggestions.
// A smaller edit distance is better, then higher frequency, then alphabetical order.
struct Better {
    bool operator()(const Suggestion& first, const Suggestion& second) const {
        if (first.distance != second.distance) {
            return first.distance < second.distance;
        }

        if (first.frequency != second.frequency) {
            return first.frequency > second.frequency;
        }

        return first.word < second.word;
    }
};

// Keep only the best K suggestions using a priority queue.
class TopK {
private:
    size_t limit;
    priority_queue<Suggestion, vector<Suggestion>, Better> heap;

public:
    explicit TopK(size_t number) : limit(number) {}

    void consider(Suggestion value) {
        if (limit == 0) {
            return;
        }

        if (heap.size() < limit) {
            heap.push(move(value));
        }
        else if (Better{}(value, heap.top())) {
            heap.pop();
            heap.push(move(value));
        }
    }

    vector<Suggestion> sorted() {
        vector<Suggestion> result;

        while (!heap.empty()) {
            result.push_back(heap.top());
            heap.pop();
        }

        sort(result.begin(), result.end(), Better{});
        return result;
    }
};

// Information returned after a search is completed.
struct SearchResult {
    vector<Suggestion> suggestions;
    size_t candidates = 0;
    size_t nodes_visited = 0;
    size_t branches_pruned = 0;
    size_t dp_cells = 0;
};

// Information returned after loading a dictionary file.
struct LoadReport {
    size_t accepted = 0;
    size_t added = 0;
    size_t updated = 0;
    size_t rejected = 0;
};

// Standard Levenshtein edit distance, used as a reference calculation.
inline int edit_distance(const string& first, const string& second) {
    vector<int> previous(second.size() + 1);
    vector<int> current(second.size() + 1);

    iota(previous.begin(), previous.end(), 0);

    for (size_t i = 1; i <= first.size(); i++) {
        current[0] = static_cast<int>(i);

        for (size_t j = 1; j <= second.size(); j++) {
            int change = (first[i - 1] == second[j - 1]) ? 0 : 1;

            current[j] = min({
                previous[j] + 1,
                current[j - 1] + 1,
                previous[j - 1] + change
            });
        }

        previous.swap(current);
    }

    return previous.back();
}

// The main class that stores words in a Trie and performs searches.
class Engine {
private:
    // Each node has 26 child positions, one for every English letter.
    // The value 'absent' means that a child does not exist.
    struct Node {
        array<uint32_t, 26> children;
        uint32_t entry = absent;

        Node() {
            children.fill(absent);
        }
    };

    // Stores the best word and total number of words below each Trie node.
    struct Ranking {
        uint32_t best = absent;
        uint32_t words = 0;
    };

    // Stores a word and its frequency in the dictionary.
    struct Entry {
        string word;
        uint64_t frequency;
    };

    // Nodes use indexes instead of pointers, so moving the vector does not
    // invalidate the links between Trie nodes.
    vector<Node> nodes{Node{}};
    vector<Entry> entries;
    vector<Ranking> ranking{Ranking{}};
    unordered_map<string, uint64_t> search_history;

    // Find the Trie node that represents a complete word or prefix.
    uint32_t locate(const string& word) const {
        uint32_t current = 0;

        for (char ch : word) {
            size_t position = static_cast<size_t>(ch - 'a');
            current = nodes[current].children[position];

            if (current == absent) {
                return absent;
            }
        }

        return current;
    }

    // Check which of two dictionary entries should appear first.
    bool better_entry(uint32_t first, uint32_t second) const {
        if (first == absent) {
            return false;
        }

        if (second == absent) {
            return true;
        }

        const Entry& firstEntry = entries[first];
        const Entry& secondEntry = entries[second];

        if (firstEntry.frequency != secondEntry.frequency) {
            return firstEntry.frequency > secondEntry.frequency;
        }

        return firstEntry.word < secondEntry.word;
    }

    // Update the ranking information of one node using its children.
    void refresh(uint32_t position) {
        Node& currentNode = nodes[position];
        Ranking& currentRank = ranking[position];

        currentRank.best = currentNode.entry;
        currentRank.words = (currentNode.entry == absent) ? 0U : 1U;

        for (uint32_t child : currentNode.children) {
            if (child == absent) {
                continue;
            }

            currentRank.words += ranking[child].words;

            if (better_entry(ranking[child].best, currentRank.best)) {
                currentRank.best = ranking[child].best;
            }
        }
    }

    // Update the nodes from the word's end back to the root.
    void refresh_path(const vector<uint32_t>& path) {
        for (auto it = path.rbegin(); it != path.rend(); ++it) {
            refresh(*it);
        }
    }

    // Check the maximum number of suggestions allowed.
    static void validate_limit(size_t limit) {
        if (limit > 100) {
            throw invalid_argument("K must be between 0 and 100.");
        }
    }

    // Check the maximum allowed edit distance.
    static void validate_distance(int distance) {
        if (distance < 0 || distance > 3) {
            throw invalid_argument("Edit limit must be 0-3.");
        }
    }

    // Visit Trie nodes for fuzzy search. The DP row is updated for each letter.
    // Branches are skipped when no word below that node can match the limit.
    void visit_fuzzy(uint32_t position, char letter, const string& query,
                     const array<int, max_word_length + 1>& previous,
                     int threshold, TopK& top, SearchResult& result) const {
        result.nodes_visited++;

        array<int, max_word_length + 1> row;
        row[0] = previous[0] + 1;

        for (size_t j = 1; j <= query.size(); j++) {
            row[j] = min({
                row[j - 1] + 1,
                previous[j] + 1,
                previous[j - 1] + (query[j - 1] == letter ? 0 : 1)
            });

            result.dp_cells++;
        }

        uint32_t entryNumber = nodes[position].entry;

        if (entryNumber != absent && row[query.size()] <= threshold) {
            const Entry& value = entries[entryNumber];
            result.candidates++;
            top.consider({value.word, value.frequency, row[query.size()]});
        }

        // If every possible partial match is already too far away, stop here.
        if (*min_element(row.begin(), row.begin() + static_cast<ptrdiff_t>(query.size() + 1)) > threshold) {
            result.branches_pruned++;
            return;
        }

        for (size_t i = 0; i < 26; i++) {
            uint32_t child = nodes[position].children[i];

            if (child != absent) {
                visit_fuzzy(child, static_cast<char>('a' + i), query,
                            row, threshold, top, result);
            }
        }
    }

public:
    // Return the number of complete words in the dictionary.
    size_t word_count() const {
        return entries.size();
    }

    // Return the number of nodes currently stored in the Trie.
    size_t node_count() const {
        return nodes.size();
    }

    // Give an approximate idea of how much memory the dictionary uses.
    // This does not include allocator overhead or search history.
    size_t estimated_storage_bytes() const {
        size_t total = nodes.capacity() * sizeof(Node) +
                       entries.capacity() * sizeof(Entry) +
                       ranking.capacity() * sizeof(Ranking);

        for (const Entry& entry : entries) {
            total += entry.word.capacity() + 1;
        }

        return total;
    }

    // Insert a new word, or update the frequency if the word already exists.
    // Returns true for a new word and false when an existing word is updated.
    bool insert(const string& text, uint64_t frequency = 1) {
        string word = normalize(text);

        if (frequency == 0 || frequency > max_frequency) {
            throw invalid_argument("Frequency must be 1-1000000000000.");
        }

        uint32_t current = 0;
        vector<uint32_t> path{0};

        for (char ch : word) {
            size_t position = static_cast<size_t>(ch - 'a');

            if (nodes[current].children[position] == absent) {
                if (nodes.size() >= absent) {
                    throw length_error("Trie capacity exceeded.");
                }

                uint32_t child = static_cast<uint32_t>(nodes.size());
                nodes.emplace_back();

                try {
                    ranking.emplace_back();
                }
                catch (...) {
                    nodes.pop_back();
                    throw;
                }

                nodes[current].children[position] = child;
            }

            current = nodes[current].children[position];
            path.push_back(current);
        }

        // If the word already exists, replace its frequency.
        if (nodes[current].entry != absent) {
            entries[nodes[current].entry].frequency = frequency;
            refresh_path(path);
            return false;
        }

        if (entries.size() >= absent) {
            throw length_error("Dictionary capacity exceeded.");
        }

        entries.push_back({word, frequency});
        nodes[current].entry = static_cast<uint32_t>(entries.size() - 1);

        refresh_path(path);
        return true;
    }

    // Check whether a complete word exists in the dictionary.
    bool contains(const string& text) const {
        uint32_t position = locate(normalize(text));
        return position != absent && nodes[position].entry != absent;
    }

    // Increase the frequency of an existing word by one.
    bool select(const string& text) {
        string word = normalize(text);
        uint32_t position = locate(word);

        if (position == absent || nodes[position].entry == absent) {
            return false;
        }

        uint64_t frequency = entries[nodes[position].entry].frequency;

        if (frequency < max_frequency) {
            insert(word, frequency + 1);
        }

        return true;
    }

    // Find words that begin with the given prefix.
    // The Trie stores the best word under each node to find top results faster.
    SearchResult autocomplete(const string& text, size_t limit = 5) const {
        validate_limit(limit);

        string prefix = normalize(text, true);
        SearchResult result;

        if (limit == 0) {
            return result;
        }

        uint32_t current = 0;

        for (char letter : prefix) {
            result.nodes_visited++;
            current = nodes[current].children[static_cast<size_t>(letter - 'a')];

            if (current == absent) {
                return result;
            }
        }

        result.candidates = ranking[current].words;

        if (ranking[current].best == absent) {
            return result;
        }

        struct Frontier {
            uint32_t node;
            uint32_t best;
            bool terminal;
        };

        auto worse = [this](const Frontier& first, const Frontier& second) {
            return better_entry(second.best, first.best);
        };

        // Expand the subtree with the best possible word first.
        priority_queue<Frontier, vector<Frontier>, decltype(worse)> frontier(worse);
        frontier.push({current, ranking[current].best, false});

        while (!frontier.empty() && result.suggestions.size() < limit) {
            Frontier item = frontier.top();
            frontier.pop();

            if (item.terminal) {
                const Entry& entry = entries[item.best];
                result.suggestions.push_back({entry.word, entry.frequency, 0});
                continue;
            }

            result.nodes_visited++;
            const Node& currentNode = nodes[item.node];

            if (currentNode.entry != absent) {
                frontier.push({item.node, currentNode.entry, true});
            }

            for (uint32_t child : currentNode.children) {
                if (child != absent && ranking[child].best != absent) {
                    frontier.push({child, ranking[child].best, false});
                }
            }
        }

        return result;
    }

    // Find words that are similar to the entered word using edit distance.
    SearchResult fuzzy(const string& text, int threshold = 2, size_t limit = 5) const {
        validate_limit(limit);
        validate_distance(threshold);

        string query = normalize(text);
        SearchResult result;

        if (limit == 0) {
            return result;
        }

        TopK top(limit);
        array<int, max_word_length + 1> initial{};
        iota(initial.begin(), initial.end(), 0);

        for (size_t i = 0; i < 26; i++) {
            uint32_t child = nodes[0].children[i];

            if (child != absent) {
                visit_fuzzy(child, static_cast<char>('a' + i), query,
                            initial, threshold, top, result);
            }
        }

        result.suggestions = top.sorted();
        return result;
    }

    // Linear prefix search. This is mainly used to compare with the Trie.
    SearchResult linear_prefix(const string& text, size_t limit = 5) const {
        validate_limit(limit);

        string prefix = normalize(text, true);
        SearchResult result;

        if (limit == 0) {
            return result;
        }

        TopK top(limit);

        for (const Entry& entry : entries) {
            if (entry.word.compare(0, prefix.size(), prefix) == 0) {
                result.candidates++;
                top.consider({entry.word, entry.frequency, 0});
            }
        }

        result.suggestions = top.sorted();
        return result;
    }

    // Linear fuzzy search used as a reference for testing and benchmarking.
    SearchResult linear_fuzzy(const string& text, int threshold = 2, size_t limit = 5) const {
        validate_limit(limit);
        validate_distance(threshold);

        string query = normalize(text);
        SearchResult result;

        if (limit == 0) {
            return result;
        }

        TopK top(limit);
        vector<int> previous(query.size() + 1);
        vector<int> current(query.size() + 1);

        for (const Entry& entry : entries) {
            size_t length_gap;

            if (entry.word.size() > query.size()) {
                length_gap = entry.word.size() - query.size();
            }
            else {
                length_gap = query.size() - entry.word.size();
            }

            if (length_gap > static_cast<size_t>(threshold)) {
                continue;
            }

            iota(previous.begin(), previous.end(), 0);
            bool cut_off = false;

            for (size_t i = 1; i <= entry.word.size(); i++) {
                current[0] = static_cast<int>(i);

                for (size_t j = 1; j <= query.size(); j++) {
                    current[j] = min({
                        previous[j] + 1,
                        current[j - 1] + 1,
                        previous[j - 1] + (entry.word[i - 1] == query[j - 1] ? 0 : 1)
                    });

                    result.dp_cells++;
                }

                previous.swap(current);

                if (*min_element(previous.begin(), previous.end()) > threshold) {
                    cut_off = true;
                    break;
                }
            }

            if (!cut_off && previous.back() <= threshold) {
                result.candidates++;
                top.consider({entry.word, entry.frequency, previous.back()});
            }
        }

        result.suggestions = top.sorted();
        return result;
    }

    // Add one to the number of times a search command has been used.
    void record_query(const string& key) {
        search_history[key]++;
    }

    // Return the query history in descending order of frequency.
    vector<pair<string, uint64_t>> history() const {
        vector<pair<string, uint64_t>> result(search_history.begin(), search_history.end());

        sort(result.begin(), result.end(), [](const auto& first, const auto& second) {
            if (first.second != second.second) {
                return first.second > second.second;
            }

            return first.first < second.first;
        });

        return result;
    }

    // Load words from a text file. A line may contain: word frequency.
    LoadReport load(const string& path) {
        ifstream file(path);

        if (!file) {
            throw runtime_error("Cannot open dictionary: " + path);
        }

        LoadReport report;
        string line;

        while (getline(file, line)) {
            size_t comment = line.find('#');

            if (comment != string::npos) {
                line.erase(comment);
            }

            istringstream fields(line);
            string word;
            string frequencyText;
            string extra;

            if (!(fields >> word)) {
                continue;
            }

            fields >> frequencyText;

            if (fields >> extra) {
                report.rejected++;
                continue;
            }

            try {
                uint64_t frequency = frequencyText.empty()
                    ? 1
                    : parse_integer(frequencyText, 1, max_frequency);

                bool added = insert(word, frequency);
                report.accepted++;

                if (added) {
                    report.added++;
                }
                else {
                    report.updated++;
                }
            }
            catch (const invalid_argument&) {
                report.rejected++;
            }
        }

        if (file.bad()) {
            throw runtime_error("Dictionary read failed: " + path);
        }

        return report;
    }

    // Save all words in alphabetical order. The original file is replaced
    // only after the temporary file has been written successfully.
    void save(const string& path) const {
        vector<Entry> snapshot = entries;

        sort(snapshot.begin(), snapshot.end(), [](const Entry& first, const Entry& second) {
            return first.word < second.word;
        });

        namespace fs = filesystem;
        fs::path target(path);

        if (target.filename().empty() || fs::is_directory(target)) {
            throw runtime_error("Save target must be a file: " + path);
        }

        fs::path parent = target.has_parent_path() ? target.parent_path() : fs::path(".");
        fs::path temporaryDirectory;

        for (int attempt = 0; attempt < 16; attempt++) {
            auto nonce = chrono::steady_clock::now().time_since_epoch().count();
            fs::path candidate = parent /
                (".triewise-save-" + to_string(nonce) + "-" + to_string(attempt));

            error_code error;

            if (fs::create_directory(candidate, error)) {
                temporaryDirectory = candidate;
                break;
            }

            if (error) {
                throw runtime_error("Cannot create save file: " + error.message());
            }
        }

        if (temporaryDirectory.empty()) {
            throw runtime_error("Cannot reserve a temporary save file.");
        }

        // Remove the temporary directory and file when this function finishes.
        struct Cleanup {
            fs::path directory;

            ~Cleanup() {
                error_code ignored;
                fs::remove(directory / "snapshot.txt", ignored);
                fs::remove(directory, ignored);
            }
        } cleanup{temporaryDirectory};

        fs::path temporaryFile = temporaryDirectory / "snapshot.txt";
        ofstream file(temporaryFile);

        if (!file) {
            throw runtime_error("Cannot write dictionary: " + path);
        }

        file << "# TrieWise dictionary: word frequency\n";

        for (const Entry& entry : snapshot) {
            file << entry.word << ' ' << entry.frequency << '\n';
        }

        file.close();

        if (!file) {
            throw runtime_error("Dictionary write failed; original file was preserved: " + path);
        }

#ifdef _WIN32
        if (!MoveFileExW(temporaryFile.c_str(), target.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            throw runtime_error(
                "Cannot replace dictionary; original file was preserved. Windows error: " +
                to_string(GetLastError())
            );
        }
#else
        error_code error;
        fs::rename(temporaryFile, target, error);

        if (error) {
            throw runtime_error(
                "Cannot replace dictionary; original file was preserved: " + error.message()
            );
        }
#endif
    }
};

// Find a value at a selected percentile in a list of timings.
inline double percentile(vector<double> values, double quantile) {
    if (values.empty()) {
        return 0;
    }

    sort(values.begin(), values.end());
    size_t index = static_cast<size_t>(quantile * static_cast<double>(values.size() - 1));
    return values[index];
}

// Compare the Trie searches with the slower linear search methods.
inline void benchmark(const Engine& engine, ostream& out, int repetitions = 20) {
    if (repetitions < 1 || repetitions > 1000) {
        throw invalid_argument("Repetitions must be 1-1000.");
    }

    if (engine.word_count() == 0) {
        throw invalid_argument("Load words before benchmarking.");
    }

    const vector<string> prefixes{"a", "co", "comp", "net", "data", "zzzz"};
    const vector<string> typos{"computr", "netwrok", "algoritm", "securty", "zzzz"};

    out << "BENCHMARK: " << engine.word_count() << " words; " << repetitions
        << " repetitions/query; K=5; fuzzy edit limit=2\n"
        << "Timing excludes file loading, history and console output. Units: microseconds.\n";

    uint64_t checksum = 0;

    auto run = [&](const string& mode, const vector<string>& queries, bool fuzzyMode) {
        out << "\n" << mode
            << "\nquery        trie median   scan median   trie p95   scan p95   scan/trie\n";

        for (const string& query : queries) {
            auto optimizedSearch = [&]() {
                return fuzzyMode ? engine.fuzzy(query) : engine.autocomplete(query);
            };

            auto normalSearch = [&]() {
                return fuzzyMode ? engine.linear_fuzzy(query) : engine.linear_prefix(query);
            };

            // First check that both methods return the same result.
            SearchResult expected = normalSearch();
            SearchResult actual = optimizedSearch();

            if (actual.suggestions != expected.suggestions ||
                actual.candidates != expected.candidates) {
                throw runtime_error("Benchmark result mismatch for " + query);
            }

            vector<double> trieTimes;
            vector<double> scanTimes;

            auto measure = [&](auto&& searchFunction, vector<double>& samples) {
                auto start = chrono::steady_clock::now();
                SearchResult result = searchFunction();
                auto finish = chrono::steady_clock::now();

                samples.push_back(chrono::duration<double, micro>(finish - start).count());
                checksum += result.candidates + result.dp_cells + result.nodes_visited;

                for (const Suggestion& value : result.suggestions) {
                    checksum += value.frequency + value.word.size();
                }
            };

            for (int i = 0; i < repetitions; i++) {
                // Alternate which method runs first to reduce timing bias.
                if (i % 2 == 0) {
                    measure(optimizedSearch, trieTimes);
                    measure(normalSearch, scanTimes);
                }
                else {
                    measure(normalSearch, scanTimes);
                    measure(optimizedSearch, trieTimes);
                }
            }

            double trieMedian = percentile(trieTimes, 0.5);
            double scanMedian = percentile(scanTimes, 0.5);

            out << left << setw(12) << query << right << fixed << setprecision(2)
                << setw(12) << trieMedian
                << setw(14) << scanMedian
                << setw(11) << percentile(trieTimes, 0.95)
                << setw(11) << percentile(scanTimes, 0.95);

            if (trieMedian > 0) {
                out << setw(12) << scanMedian / trieMedian;
            }
            else {
                out << "         n/a";
            }

            out << '\n';
        }
    };

    run("PREFIX", prefixes, false);
    run("FUZZY", typos, true);

    out << "\nParity: PASS (all queries). Checksum: " << checksum
        << "\nA ratio below 1 means the linear scan was faster for that query.\n";
}

} // namespace triewise
