#include "triewise.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <cstdint>
#include <cstddef>

using namespace std;
using triewise::Engine;

namespace {

// Display all available commands.
void help(bool in_menu = false) {
    cout << "\nCommands (press Enter after each command):\n"
         << "  prefix TEXT [K]       Prefix completions; K defaults to 5\n"
         << "  prefix * [K]          Top words across the dictionary\n"
         << "  fuzzy WORD [D] [K]    Whole-word typo correction; D=2, K=5\n"
         << "  search WORD [K]       Prefix first; fuzzy fallback if no prefix matches\n"
         << "  exact WORD            Check whether a full word exists\n"
         << "  add WORD [FREQUENCY]  Insert a word, or replace its frequency\n"
         << "  select WORD           Increase an existing word's frequency by one\n"
         << "  load \"PATH\"           Merge a dictionary; duplicate words replace frequency\n"
         << "  save \"PATH\"           Save all words and frequencies (overwrites PATH)\n"
         << "  history               Show successful search commands and usage counts\n"
         << "  stats                 Dictionary size and approximate storage\n"
         << "  benchmark [RUNS]      Compare Trie with linear scan; RUNS defaults to 20\n"
         << "  help                  Show this menu\n"
         << "  quit                  Exit\n"
         << "\nEnglish letters only; 64 letters/word; edit limit 0-3; K=0-100.\n";

    if (in_menu) {
        cout << "  back                  Return to the simple menu\n";
    }
}

// Read a required word or file path from the command.
string getRequiredArgument(istringstream& command) {
    string value;

    // Keep backslashes in quoted Windows file paths.
    if (!(command >> quoted(value, '"', '\0'))) {
        throw invalid_argument("Missing argument. Type help for usage.");
    }

    return value;
}

// Read an optional number. Use the default value if it was not entered.
uint64_t getOptionalNumber(istringstream& command, uint64_t defaultValue,
                           uint64_t minimum, uint64_t maximum) {
    string value;

    if (!(command >> value)) {
        return defaultValue;
    }

    return triewise::parse_integer(value, minimum, maximum);
}

// Check that the user has not entered extra arguments.
void checkExtraArguments(istringstream& command) {
    string extra;

    if (command >> extra) {
        throw invalid_argument("Too many arguments. Type help for usage.");
    }
}

// Display the suggestions returned by the Trie.
void displaySearchResults(const triewise::SearchResult& result,
                          double elapsedTime, bool isFuzzy) {
    if (result.suggestions.empty()) {
        cout << "No suggestions.\n";
    }

    for (size_t i = 0; i < result.suggestions.size(); i++) {
        const auto& suggestion = result.suggestions[i];

        cout << "  " << i + 1 << ". " << suggestion.word
             << "  frequency=" << suggestion.frequency;

        if (isFuzzy) {
            cout << "  edits=" << suggestion.distance;
        }

        cout << '\n';
    }

    cout << "Candidates: " << result.candidates
         << " | Node visits: " << result.nodes_visited;

    if (isFuzzy) {
        cout << " | Cut-off branches: " << result.branches_pruned
             << " | DP cells: " << result.dp_cells;
    }

    cout << " | " << fixed << setprecision(2) << elapsedTime << " us\n";
}

// Display the result of loading a dictionary file.
void displayLoadReport(const triewise::LoadReport& report) {
    cout << "Loaded " << report.accepted << " records: "
         << report.added << " new, "
         << report.updated << " updated, "
         << report.rejected << " rejected.\n";
}

// Run the advanced command mode.
// Returns true when the program should exit and false when the user selects back.
bool commandMode(Engine& engine, bool& unsavedChanges,
                 string& savePath, bool inMenu) {
    help(inMenu);

    string line;

    while (cout << "\ntriewise> " && getline(cin, line)) {
        try {
            istringstream command(line);
            string action;

            if (!(command >> action)) {
                continue;
            }

            if (action == "quit" || action == "exit") {
                checkExtraArguments(command);
                return true;
            }

            if (inMenu && action == "back") {
                checkExtraArguments(command);
                return false;
            }

            if (action == "help") {
                checkExtraArguments(command);
                help(inMenu);
            }
            else if (action == "prefix" || action == "fuzzy" || action == "search") {
                string query = getRequiredArgument(command);

                if (action == "prefix" && query == "*") {
                    query.clear();
                }

                query = triewise::normalize(query, action == "prefix");

                int distance = 2;
                if (action == "fuzzy") {
                    distance = static_cast<int>(getOptionalNumber(command, 2, 0, 3));
                }

                size_t limit = static_cast<size_t>(getOptionalNumber(command, 5, 0, 100));
                checkExtraArguments(command);

                bool isFuzzy = (action == "fuzzy");
                auto startTime = chrono::steady_clock::now();

                triewise::SearchResult result;
                if (isFuzzy) {
                    result = engine.fuzzy(query, distance, limit);
                }
                else {
                    result = engine.autocomplete(query, limit);
                }

                // If normal prefix search gives no result, try fuzzy search.
                if (action == "search" && result.candidates == 0 && limit != 0) {
                    isFuzzy = true;
                    result = engine.fuzzy(query, 2, limit);
                }

                auto endTime = chrono::steady_clock::now();

                if (action == "search") {
                    if (isFuzzy) {
                        cout << "Fuzzy fallback\n";
                    }
                    else {
                        cout << "Prefix matches\n";
                    }
                }

                double elapsedTime = chrono::duration<double, micro>(endTime - startTime).count();
                displaySearchResults(result, elapsedTime, isFuzzy);

                engine.record_query(action + " " + (query.empty() ? "*" : query) +
                    " d=" + to_string(distance) + " k=" + to_string(limit));
            }
            else if (action == "exact") {
                string word = getRequiredArgument(command);
                checkExtraArguments(command);

                if (engine.contains(word)) {
                    cout << "Word exists.\n";
                }
                else {
                    cout << "Word not found.\n";
                }
            }
            else if (action == "add") {
                string word = getRequiredArgument(command);
                uint64_t frequency = getOptionalNumber(command, 1, 1, triewise::max_frequency);
                checkExtraArguments(command);

                if (engine.insert(word, frequency)) {
                    cout << "Word added.\n";
                }
                else {
                    cout << "Frequency replaced.\n";
                }

                unsavedChanges = true;
            }
            else if (action == "select") {
                string word = getRequiredArgument(command);
                checkExtraArguments(command);

                bool found = engine.select(word);

                if (found) {
                    cout << "Frequency updated (saturates at maximum).\n";
                    unsavedChanges = true;
                }
                else {
                    cout << "Word not found.\n";
                }
            }
            else if (action == "load" || action == "save") {
                string path = getRequiredArgument(command);
                checkExtraArguments(command);

                if (action == "load") {
                    // Loading may partially update the dictionary before an error occurs.
                    unsavedChanges = true;
                    triewise::LoadReport report = engine.load(path);
                    displayLoadReport(report);

                    if (report.accepted != 0) {
                        unsavedChanges = true;
                    }
                }
                else {
                    engine.save(path);
                    unsavedChanges = false;
                    savePath = path;
                    cout << "Saved " << engine.word_count() << " words.\n";
                }
            }
            else if (action == "stats") {
                checkExtraArguments(command);

                cout << "Words: " << engine.word_count()
                     << " | Nodes: " << engine.node_count()
                     << " | Estimated dictionary storage: "
                     << engine.estimated_storage_bytes() << " bytes\n"
                     << "Estimate excludes allocator overhead and query history; not process RAM.\n";
            }
            else if (action == "history") {
                checkExtraArguments(command);

                auto searchHistory = engine.history();

                if (searchHistory.empty()) {
                    cout << "No searches recorded.\n";
                }

                for (const auto& item : searchHistory) {
                    cout << item.second << " x " << item.first << '\n';
                }
            }
            else if (action == "benchmark") {
                uint64_t runs = getOptionalNumber(command, 20, 1, 1000);
                checkExtraArguments(command);
                triewise::benchmark(engine, cout, static_cast<int>(runs));
            }
            else {
                cout << "Unknown command. Type help.\n";
            }
        }
        catch (const exception& error) {
            cout << "Error: " << error.what() << '\n';
        }
    }

    return true;
}

// Remove extra spaces from the beginning and end of user input.
string trimInput(const string& text) {
    size_t first = text.find_first_not_of(" \t\r\n");

    if (first == string::npos) {
        return "";
    }

    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Ask the user for input and clean the entered text.
bool askInput(const string& prompt, string& answer) {
    cout << prompt;

    if (!getline(cin, answer)) {
        return false;
    }

    answer = trimInput(answer);
    return true;
}

// Remove quotation marks around a file path, if the user entered them.
string removeQuotes(const string& text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        return text.substr(1, text.size() - 2);
    }

    return text;
}

// Save the dictionary to the selected file.
bool saveDictionary(Engine& engine, bool& hasUnsavedChanges, string& savePath) {
    string answer;

    if (!askInput("Save to [" + savePath + "] (Enter uses this path): ", answer)) {
        return false;
    }

    string path = answer.empty() ? savePath : removeQuotes(answer);

    // Update the saved path and status only when saving succeeds.
    engine.save(path);
    savePath = path;
    hasUnsavedChanges = false;

    cout << "Saved " << engine.word_count() << " words to: " << path << '\n';
    return true;
}

// Ask the user whether they want to save before exiting.
bool confirmExit(Engine& engine, bool& hasUnsavedChanges, string& savePath) {
    if (!hasUnsavedChanges) {
        return true;
    }

    string answer;

    while (askInput("Save your changes? [Y/n/c] (Enter=Yes, c=Cancel): ", answer)) {
        if (answer.empty() || answer == "y" || answer == "Y") {
            return saveDictionary(engine, hasUnsavedChanges, savePath) || !cin;
        }

        if (answer == "n" || answer == "N") {
            return true;
        }

        if (answer == "c" || answer == "C") {
            return false;
        }

        cout << "Enter y to save, n to discard, or c to keep working.\n";
    }

    return true;
}

// Search for a word and allow the user to select one of the suggestions.
void searchMenu(Engine& engine, bool& hasUnsavedChanges, bool spellingCorrection) {
    string query;

    if (!askInput("Enter a word (or its beginning); Enter alone goes back: ", query) || query.empty()) {
        return;
    }

    query = triewise::normalize(query);

    bool isFuzzy = spellingCorrection;
    triewise::SearchResult result;

    if (isFuzzy) {
        result = engine.fuzzy(query);
    }
    else {
        result = engine.autocomplete(query);
    }

    // Try spelling correction if prefix search does not find any words.
    if (!isFuzzy && result.candidates == 0) {
        isFuzzy = true;
        result = engine.fuzzy(query);
        cout << "No words start with that. Trying spelling suggestions.\n";
    }

    engine.record_query(string(spellingCorrection ? "fuzzy " : "search ") + query + " d=2 k=5");

    if (result.suggestions.empty()) {
        cout << "No matching words found.\n";
        return;
    }

    if (isFuzzy) {
        cout << "Spelling suggestions (up to 2 letter edits):\n";
    }
    else {
        cout << "Words starting with " << query << ":\n";
    }

    for (size_t i = 0; i < result.suggestions.size(); i++) {
        const auto& word = result.suggestions[i];

        cout << "  " << i + 1 << ". " << word.word;

        if (isFuzzy) {
            cout << " (" << word.distance;
            if (word.distance == 1) {
                cout << " edit)";
            }
            else {
                cout << " edits)";
            }
        }

        cout << '\n';
    }

    string choice;

    while (askInput("Choose a result number to boost its ranking, or Enter to go back: ", choice)) {
        if (choice.empty()) {
            return;
        }

        try {
            size_t index = static_cast<size_t>(triewise::parse_integer(
                choice, 1, result.suggestions.size()));

            string selectedWord = result.suggestions[index - 1].word;
            engine.select(selectedWord);
            hasUnsavedChanges = true;

            cout << "Selected " << selectedWord
                 << ". Its ranking weight increased by one (up to the maximum).\n";
            return;
        }
        catch (const invalid_argument&) {
            cout << "Choose 1-" << result.suggestions.size()
                 << ", or press Enter to go back.\n";
        }
    }
}

// Show extra features that are not part of the main menu.
bool moreOptionsMenu(Engine& engine, bool& hasUnsavedChanges, string& savePath) {
    string choice;

    while (askInput("\nMore options\n  1. Check an exact word\n  2. Load another dictionary\n"
                    "  3. Search history\n  4. Dictionary statistics\n"
                    "  5. Advanced commands and benchmarks\n  0. Back\nChoose 0-5: ", choice)) {
        try {
            if (choice == "0" || choice.empty()) {
                return false;
            }
            else if (choice == "1") {
                string word;

                if (askInput("Word (Enter to cancel): ", word) && !word.empty()) {
                    if (engine.contains(word)) {
                        cout << "Word exists.\n";
                    }
                    else {
                        cout << "Word not found.\n";
                    }
                }
            }
            else if (choice == "2") {
                string path;

                if (askInput("File to merge (Enter to cancel; duplicate weights are replaced): ", path)
                    && !path.empty()) {
                    // Loading can update some records before an error occurs.
                    hasUnsavedChanges = true;
                    displayLoadReport(engine.load(removeQuotes(path)));
                }
            }
            else if (choice == "3") {
                auto searchHistory = engine.history();

                if (searchHistory.empty()) {
                    cout << "No searches recorded.\n";
                }

                for (const auto& item : searchHistory) {
                    cout << item.second << " x " << item.first << '\n';
                }
            }
            else if (choice == "4") {
                cout << "Words: " << engine.word_count()
                     << " | Trie nodes: " << engine.node_count()
                     << "\nApproximate dictionary storage: "
                     << engine.estimated_storage_bytes()
                     << " bytes (excludes history and allocator overhead).\n";
            }
            else if (choice == "5") {
                if (commandMode(engine, hasUnsavedChanges, savePath, true)) {
                    return true;
                }
            }
            else {
                cout << "Please choose a number from 0 to 5.\n";
            }
        }
        catch (const exception& error) {
            cout << "Error: " << error.what() << '\n';
        }
    }

    return true;
}

// Display and handle the main numbered menu.
void mainMenu(Engine& engine, bool& hasUnsavedChanges, string& savePath) {
    cout << "Use the numbers below. Searches show up to five suggestions.\n";

    string choice;

    while (askInput("\n  1. Search for a word\n  2. Correct spelling\n  3. Add a word\n"
                    "  4. Save dictionary\n  5. More options\n  0. Exit\nChoose 0-5: ", choice)) {
        try {
            if (choice == "0") {
                if (confirmExit(engine, hasUnsavedChanges, savePath)) {
                    return;
                }
            }
            else if (choice == "1" || choice == "2") {
                searchMenu(engine, hasUnsavedChanges, choice == "2");
            }
            else if (choice == "3") {
                string word;

                if (askInput("New word (English letters only; Enter to cancel): ", word)
                    && !word.empty()) {
                    word = triewise::normalize(word);

                    if (engine.contains(word)) {
                        cout << "That word already exists. Its weight was kept.\n";
                    }
                    else {
                        engine.insert(word);
                        hasUnsavedChanges = true;
                        cout << "Added " << word << ".\n";
                    }
                }
            }
            else if (choice == "4") {
                saveDictionary(engine, hasUnsavedChanges, savePath);
            }
            else if (choice == "5") {
                if (moreOptionsMenu(engine, hasUnsavedChanges, savePath)
                    && confirmExit(engine, hasUnsavedChanges, savePath)) {
                    return;
                }
            }
            else if (!choice.empty()) {
                cout << "Please choose a number from 0 to 5.\n";
            }
        }
        catch (const exception& error) {
            cout << "Error: " << error.what() << '\n';
        }
    }
}

// Find the default dictionary file if the user did not provide a path.
string findDefaultDictionary(const char* executable) {
    namespace fs = std::filesystem;

    fs::path programFolder = fs::absolute(executable).parent_path();

    for (const auto& folder : {fs::path("data"), programFolder / "data",
                               programFolder.parent_path() / "data"}) {
        if (fs::exists(folder / "my_dictionary.txt")) {
            return (folder / "my_dictionary.txt").string();
        }

        if (fs::exists(folder / "dictionary.txt")) {
            return (folder / "dictionary.txt").string();
        }
    }

    return "data/dictionary.txt";
}

const char* usage = "Usage: triewise [dictionary.txt] [--commands | --benchmark [repetitions]]";

} // End of unnamed namespace

int main(int argc, char* argv[]) {
    try {
        if (argc > 1 && string(argv[1]) == "--help") {
            if (argc != 2) {
                throw invalid_argument("--help does not accept extra arguments.");
            }

            cout << usage << "\nDefault: simple numbered menu.\n"
                 << "Default dictionary: data/my_dictionary.txt if present, otherwise data/dictionary.txt.\n";
            help();
            return 0;
        }

        int nextArgument = 1;
        string dictionary;

        if (nextArgument < argc && string(argv[nextArgument]).rfind("--", 0) != 0) {
            dictionary = argv[nextArgument];
            nextArgument++;
        }

        bool benchmarkMode = false;
        bool commandModeEnabled = false;
        int repetitions = 20;

        if (nextArgument < argc) {
            string mode = argv[nextArgument];
            nextArgument++;

            if (mode == "--commands") {
                commandModeEnabled = true;
            }
            else if (mode == "--benchmark") {
                benchmarkMode = true;

                if (nextArgument < argc) {
                    repetitions = static_cast<int>(
                        triewise::parse_integer(argv[nextArgument], 1, 1000));
                    nextArgument++;
                }
            }
            else {
                throw invalid_argument(usage);
            }
        }

        if (nextArgument != argc) {
            throw invalid_argument(usage);
        }

        if (dictionary.empty()) {
            dictionary = findDefaultDictionary(argv[0]);
        }

        Engine engine;

        auto startTime = chrono::steady_clock::now();
        triewise::LoadReport report = engine.load(dictionary);
        auto endTime = chrono::steady_clock::now();

        cout << "\nTrieWise | Autocomplete and spelling suggestions\n"
             << "Using " << engine.word_count() << " words from: " << dictionary << '\n';

        if (report.rejected) {
            cout << "Skipped " << report.rejected << " invalid dictionary records.\n";
        }

        if (benchmarkMode || commandModeEnabled) {
            displayLoadReport(report);

            double loadTime = chrono::duration<double, milli>(endTime - startTime).count();
            cout << "Construction: " << fixed << setprecision(2) << loadTime << " ms\n";
        }

        if (benchmarkMode) {
            triewise::benchmark(engine, cout, repetitions);
            return 0;
        }

        bool hasUnsavedChanges = false;
        string savePath = (filesystem::path(dictionary).parent_path() / "my_dictionary.txt").string();

        if (commandModeEnabled) {
            commandMode(engine, hasUnsavedChanges, savePath, false);
        }
        else {
            mainMenu(engine, hasUnsavedChanges, savePath);
        }

        if (hasUnsavedChanges) {
            cout << "Note: dictionary changes were not saved.\n";
        }

        cout << "Goodbye.\n";
        return 0;
    }
    catch (const exception& error) {
        cerr << "Error: " << error.what()
             << "\nRun from the project folder, or pass a dictionary path. Use --help for usage.\n";
        return 1;
    }
}
