#include "triewise.hpp"
#include <filesystem>
#include <iostream>
#include <random>

namespace {
std::size_t checks = 0;
void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) throw std::runtime_error("FAILED: " + message);
}
template<class Function> void rejects(Function action, const std::string& message) {
    bool failed = false;
    try { action(); } catch (const std::invalid_argument&) { failed = true; }
    check(failed, message);
}
int matrix_distance(const std::string& a, const std::string& b) {
    std::vector<std::vector<int>> matrix(a.size() + 1, std::vector<int>(b.size() + 1));
    for (std::size_t i = 0; i <= a.size(); ++i) matrix[i][0] = static_cast<int>(i);
    for (std::size_t j = 0; j <= b.size(); ++j) matrix[0][j] = static_cast<int>(j);
    for (std::size_t i = 1; i <= a.size(); ++i)
        for (std::size_t j = 1; j <= b.size(); ++j)
            matrix[i][j] = std::min({matrix[i-1][j] + 1, matrix[i][j-1] + 1,
                                     matrix[i-1][j-1] + (a[i-1] != b[j-1])});
    return matrix.back().back();
}
void words(std::string prefix, int remaining, std::vector<std::string>& result) {
    if (!prefix.empty()) result.push_back(prefix);
    if (remaining == 0) return;
    for (char c : std::string("abc")) words(prefix + c, remaining - 1, result);
}
void parity(const triewise::Engine& engine, const std::string& query, int distance, std::size_t k) {
    const auto actual = engine.fuzzy(query, distance, k);
    const auto expected = engine.linear_fuzzy(query, distance, k);
    check(actual.suggestions == expected.suggestions, "fuzzy results: " + query);
    check(actual.candidates == expected.candidates, "fuzzy candidates: " + query);
    const auto prefix = engine.autocomplete(query, k);
    const auto linear = engine.linear_prefix(query, k);
    check(prefix.suggestions == linear.suggestions, "prefix results: " + query);
    check(prefix.candidates == linear.candidates, "prefix candidates: " + query);
}
} // namespace

int main() {
    try {
        using namespace triewise;
        Engine engine;
        check(engine.word_count() == 0 && engine.node_count() == 1, "empty dictionary");
        check(engine.autocomplete("").suggestions.empty(), "empty prefix/empty dictionary");
        check(engine.fuzzy("a").suggestions.empty(), "empty dictionary fuzzy");
        check(engine.insert("Computer", 10), "insert");
        check(engine.contains("COMPUTER"), "case normalization");
        check(!engine.contains("comp"), "prefix is not a full word");
        check(!engine.insert("computer", 15) && engine.word_count() == 1, "duplicate replacement");
        engine.insert("compile", 15); engine.insert("computation", 5); engine.insert("computing", 8);
        const auto top = engine.autocomplete("comp", 2).suggestions;
        check(top.size() == 2 && top[0].word == "compile" && top[1].word == "computer", "heap tie ranking");
        check(engine.select("computer"), "selection");
        check(engine.autocomplete("comp", 1).suggestions[0].word == "computer", "ranking update");
        engine.insert("computer", 1);
        check(engine.autocomplete("comp", 1).suggestions[0].word == "compile", "best-word metadata after frequency decrease");
        engine.insert("computer", 16);
        check(!engine.select("absent"), "select missing word");
        check(engine.fuzzy("computr", 1).suggestions[0].word == "computer", "single deletion");
        check(edit_distance("computr", "computing") == 3, "proposal correction");
        check(edit_distance("kitten", "sitting") == 3, "classic example");
        check(edit_distance("", "abc") == 3 && edit_distance("abc", "") == 3, "empty distance");
        engine.insert("a", 100); engine.insert("ab", 1);
        check(engine.fuzzy("ab", 1).suggestions[0].word == "ab", "distance outranks frequency");
        engine.insert("maximum", max_frequency); engine.select("maximum");
        check(engine.autocomplete("maximum").suggestions[0].frequency == max_frequency, "frequency saturation");
        check(engine.autocomplete("", 0).suggestions.empty(), "K zero");
        check(engine.fuzzy("x", 0, 0).suggestions.empty(), "fuzzy K zero");
        check(engine.autocomplete("missing").suggestions.empty(), "unknown prefix");
        rejects([&] { engine.insert(""); }, "empty word rejected");
        rejects([&] { engine.insert("hello-world"); }, "punctuation rejected");
        rejects([&] { engine.insert("two words"); }, "spaces rejected");
        rejects([&] { engine.insert(std::string(1, static_cast<char>(255))); }, "non-ASCII rejected");
        rejects([&] { engine.insert("zero", 0); }, "zero frequency rejected");
        rejects([&] { engine.insert("overflow", max_frequency + 1); }, "large frequency rejected");
        rejects([&] { engine.insert(std::string(65, 'a')); }, "long word rejected");
        engine.insert(std::string(64, 'z'), 2);
        check(engine.contains(std::string(64, 'z')), "maximum word length");
        parity(engine, std::string(64, 'z'), 3, 5);
        rejects([&] { engine.fuzzy(""); }, "empty fuzzy query");
        rejects([&] { engine.fuzzy("a", -1); }, "negative edit distance");
        rejects([&] { engine.fuzzy("a", 4); }, "large edit distance");
        rejects([&] { engine.autocomplete("a", 101); }, "large K");
        rejects([&] { parse_integer("-1", 0, 5); }, "negative numeric input");
        rejects([&] { parse_integer("2abc", 0, 5); }, "numeric suffix");
        rejects([&] { parse_integer("99999999999999999999999999999", 0, 5); }, "numeric overflow");
        engine.record_query("prefix comp"); engine.record_query("prefix comp");
        engine.record_query("fuzzy computr");
        check(engine.history()[0] == std::make_pair(std::string("prefix comp"), std::uint64_t{2}), "hash history");

        const auto directory = std::filesystem::temp_directory_path() /
            ("triewise-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(directory);
        const auto input = directory / "input.txt";
        { std::ofstream file(input); file << "# comment\nAPPLE 8\nbanana\napple 9\ninvalid! 2\n"
            "zero 0\nnegative -1\nextra 2 x\noverflow 99999999999999999999999\n\npear 4 # note\n"; }
        Engine loaded;
        const auto report = loaded.load(input.string());
        check(report.accepted == 4 && report.added == 3 && report.updated == 1 && report.rejected == 5,
              "load counts, comments, duplicates and malformed rows");
        check(loaded.autocomplete("apple").suggestions[0].frequency == 9, "duplicate last record wins");
        const auto output = directory / "roundtrip.txt";
        loaded.save(output.string());
        Engine restored; restored.load(output.string());
        check(loaded.autocomplete("", 100).suggestions == restored.autocomplete("", 100).suggestions,
              "save/load round trip");
        loaded.insert("apple", 1); loaded.insert("newword", 500);
        loaded.save(output.string());
        Engine replaced; replaced.load(output.string());
        check(replaced.contains("newword") && replaced.autocomplete("apple").suggestions[0].frequency == 1,
              "safe replacement of existing snapshot");
        const auto protected_directory = directory / "protected";
        std::filesystem::create_directory(protected_directory);
        const auto sentinel = protected_directory / "keep.txt";
        { std::ofstream file(sentinel); file << "original"; }
        bool directory_rejected = false;
        try { loaded.save(protected_directory.string()); } catch (const std::runtime_error&) { directory_rejected = true; }
        check(directory_rejected && std::filesystem::exists(sentinel), "failed save preserves existing destination");
        std::size_t temporary_count = 0;
        for (const auto& file : std::filesystem::directory_iterator(directory))
            if (file.path().filename().string().find(".triewise-save-") == 0) ++temporary_count;
        check(temporary_count == 0, "temporary save cleanup");
        std::filesystem::remove(sentinel); std::filesystem::remove(protected_directory);
        bool io_failed = false;
        try { loaded.load((directory / "missing.txt").string()); } catch (const std::runtime_error&) { io_failed = true; }
        check(io_failed, "missing file error");
        io_failed = false;
        try { loaded.save((directory / "missing" / "out.txt").string()); } catch (const std::runtime_error&) { io_failed = true; }
        check(io_failed, "unwritable path error");
        std::filesystem::remove(input); std::filesystem::remove(output); std::filesystem::remove(directory);

        std::vector<std::string> small;
        words("", 4, small);
        Engine exhaustive;
        for (std::size_t i = 0; i < small.size(); ++i) exhaustive.insert(small[i], i % 7 + 1);
        for (const auto& a : small) {
            for (const auto& b : small) check(edit_distance(a, b) == matrix_distance(a, b), "DP versus full matrix");
            for (int distance = 0; distance <= 3; ++distance)
                for (std::size_t k : {std::size_t{0}, std::size_t{1}, std::size_t{5}, std::size_t{100}}) {
                    parity(exhaustive, a, distance, k);
                    // Independent full collection + full sort checks the heap/ranking,
                    // not just parity between two searches sharing the same heap.
                    std::vector<Suggestion> expected_fuzzy, expected_prefix;
                    for (std::size_t i = 0; i < small.size(); ++i) {
                        const auto edits = matrix_distance(small[i], a);
                        if (edits <= distance) expected_fuzzy.push_back({small[i], i % 7 + 1, edits});
                        if (small[i].compare(0, a.size(), a) == 0)
                            expected_prefix.push_back({small[i], i % 7 + 1, 0});
                    }
                    auto order = [](const Suggestion& x, const Suggestion& y) {
                        if (x.distance < y.distance) return true;
                        if (x.distance > y.distance) return false;
                        if (x.frequency > y.frequency) return true;
                        if (x.frequency < y.frequency) return false;
                        return x.word < y.word;
                    };
                    std::sort(expected_fuzzy.begin(), expected_fuzzy.end(), order);
                    std::sort(expected_prefix.begin(), expected_prefix.end(), order);
                    expected_fuzzy.resize(std::min(k, expected_fuzzy.size()));
                    expected_prefix.resize(std::min(k, expected_prefix.size()));
                    check(exhaustive.fuzzy(a, distance, k).suggestions == expected_fuzzy, "fuzzy full-sort oracle");
                    check(exhaustive.autocomplete(a, k).suggestions == expected_prefix, "prefix full-sort oracle");
                }
        }
        std::mt19937 random(20261016);
        auto random_word = [&]() {
            std::string word;
            const auto length = 1 + random() % 16;
            for (std::uint32_t i = 0; i < length; ++i) word += static_cast<char>('a' + random() % 26);
            return word;
        };
        Engine large;
        for (int i = 0; i < 2000; ++i) large.insert(random_word(), 1 + random() % 1000);
        for (int i = 0; i < 250; ++i) parity(large, random_word(), i % 4, static_cast<std::size_t>(i % 11));
        // Updates must refresh ancestor bounds for both increases and decreases.
        for (int i = 0; i < 200; ++i) {
            const auto& word = small[static_cast<std::size_t>(i) % small.size()];
            exhaustive.insert(word, 1 + random() % 100);
            exhaustive.select(word);
            parity(exhaustive, word.substr(0, 1), i % 4, static_cast<std::size_t>(i % 11));
        }
        check(exhaustive.autocomplete("", 100).suggestions == exhaustive.linear_prefix("", 100).suggestions,
              "empty prefix ranking");
        std::ostringstream bench;
        benchmark(engine, bench, 2);
        check(bench.str().find("Parity: PASS") != std::string::npos, "benchmark parity");
        std::cout << "PASS: " << checks << " checks (examples, exhaustive, randomized, persistence, benchmark).\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
