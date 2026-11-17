#!/usr/bin/env python3
"""End-to-end tests of the actual compiled CLI; Python is test-only."""
import pathlib
import os
import subprocess
import sys
import tempfile

binary = str(pathlib.Path(sys.argv[1]).resolve())
dictionary = str(pathlib.Path(sys.argv[2]).resolve())
checks = 0

def check(condition, message):
    global checks
    checks += 1
    if not condition:
        raise AssertionError(message)

with tempfile.TemporaryDirectory(prefix='triewise-cli-') as folder:
    # On POSIX a literal backslash catches accidental Windows-path unescaping.
    saved = pathlib.Path(folder) / ('saved dictionary\\test.txt' if os.name != 'nt' else 'saved dictionary.txt')
    commands = '\n'.join([
        'prefix comp 5', 'fuzzy computr 1 5', 'search computr 5',
        'exact COMPUTER', 'exact nonexistentword', 'add uniqueexample 100000',
        'select uniqueexample', 'prefix unique 1', 'add apple -1',
        'fuzzy abc 4', 'prefix abc 101', 'prefix abc -1', 'prefix bad!',
        'fuzzy ""', 'add', 'stats extra', 'unknown', 'add extra 10 junk',
        'prefix * 0', 'fuzzy abc 0 0', 'history', 'stats',
        f'save "{saved}"', 'quit', ''
    ])
    process = subprocess.run([binary, dictionary, '--commands'], input=commands, text=True, capture_output=True, timeout=30)
    check(process.returncode == 0, process.stderr)
    text = process.stdout
    check('computer' in text and 'edits=1' in text, 'fuzzy output')
    check('Fuzzy fallback' in text, 'combined search fallback')
    check('Word exists.' in text and 'Word not found.' in text, 'exact lookup')
    check('frequency=100001' in text, 'selection frequency')
    check(text.count('Error:') == 9, 'malformed commands rejected without exiting: ' + text)
    check('Unknown command' in text, 'unknown command')
    check('Saved ' in text and saved.exists(), 'save with spaces in path')
    reloaded = subprocess.run([binary, str(saved), '--commands'], input='exact uniqueexample\nprefix unique\nquit\n',
                              text=True, capture_output=True, timeout=30)
    check(reloaded.returncode == 0 and 'frequency=100001' in reloaded.stdout, 'saved file round trip')
    eof = subprocess.run([binary, dictionary, '--commands'], input='', text=True, capture_output=True, timeout=10)
    check(eof.returncode == 0 and 'Goodbye.' in eof.stdout, 'EOF exit')
    missing = subprocess.run([binary, str(pathlib.Path(folder) / 'missing.txt')], capture_output=True, text=True)
    check(missing.returncode == 1 and 'Cannot open dictionary' in missing.stderr, 'missing startup file')
    invalid = subprocess.run([binary, dictionary, '--benchmark', '0'], capture_output=True, text=True)
    check(invalid.returncode == 1, 'invalid benchmark argument')
    benchmark = subprocess.run([binary, dictionary, '--benchmark', '2'], capture_output=True, text=True, timeout=30)
    check(benchmark.returncode == 0 and 'Parity: PASS' in benchmark.stdout, 'benchmark mode')
    help_result = subprocess.run([binary, '--help'], capture_output=True, text=True)
    check(help_result.returncode == 0 and 'Usage:' in help_result.stdout, 'startup help')
    bad_help = subprocess.run([binary, '--help', 'extra'], capture_output=True, text=True)
    check(bad_help.returncode == 1, 'strict help arguments')
    unsaved = subprocess.run([binary, dictionary, '--commands'], input='add unsavedexample 1\nquit\n',
                             capture_output=True, text=True, timeout=10)
    check('changes were not saved' in unsaved.stdout, 'unsaved changes notice')
    check('changes were not saved' not in text, 'successful save clears unsaved notice')

    def menu(commands, source=dictionary, cwd=None):
        return subprocess.run([binary, source], input=commands, text=True,
                              capture_output=True, timeout=15, cwd=cwd)

    simple = menu('1\n  COMP  \n\n2\ncomputr\n\n1\nnetwrok\n\n0\n')
    check(simple.returncode == 0 and 'Words starting with comp:' in simple.stdout, 'simple search and trimming')
    check('Spelling suggestions' in simple.stdout and 'computer (1 edit)' in simple.stdout,
          'simple spelling corrections')
    check('No words start with that' in simple.stdout and 'network (2 edits)' in simple.stdout,
          'simple automatic fallback')
    check('Node visits:' not in simple.stdout and 'Construction:' not in simple.stdout,
          'simple mode hides technical output')
    check('Save your changes?' not in simple.stdout, 'search alone leaves dictionary unchanged')

    chosen = pathlib.Path(folder) / 'menu saved file.txt'
    flow = menu(f'1\ncomp\n99\nabc\n1\n3\nmenuword\n3\nCOMPUTER\n4\n{chosen}\n0\n')
    check(flow.returncode == 0 and flow.stdout.count('Choose 1-5') == 2, 'invalid result choices retry')
    check('Selected computer' in flow.stdout and 'Added menuword' in flow.stdout, 'result selection and insertion')
    check('already exists. Its weight was kept.' in flow.stdout, 'adding duplicate does not reset weight')
    check('computer 9801' in chosen.read_text() and 'menuword 1' in chosen.read_text(), 'menu save persists changes')
    check('Save your changes?' not in flow.stdout, 'explicit menu save clears dirty flag')

    invalid_menu = menu('99\nno\n1\nbad!\n3\nbad123\n1\nqqqqqqqqqqqqqqqq\n0\n')
    check(invalid_menu.stdout.count('Please choose a number') == 2 and invalid_menu.stdout.count('Error:') == 2,
          'invalid menu choices and words recover')
    check('No matching words found.' in invalid_menu.stdout and 'Goodbye.' in invalid_menu.stdout, 'no-match flow')
    cancel = menu('3\ncancelword\n0\nc\n0\nmaybe\nn\n')
    check('Enter y to save' in cancel.stdout and 'changes were not saved' in cancel.stdout,
          'cancel exit, invalid confirmation and discard')
    interrupted = menu('3\neofword\n0\n')
    check(interrupted.returncode == 0 and 'changes were not saved' in interrupted.stdout, 'EOF during save prompt')
    during_search = menu('1\ncomp\n')
    check(during_search.returncode == 0 and 'Goodbye.' in during_search.stdout, 'EOF during selection')

    save_on_exit = pathlib.Path(folder) / 'exit saved.txt'
    exiting = menu(f'3\nexitword\n0\n\n{save_on_exit}\n')
    check(save_on_exit.exists() and 'exitword 1' in save_on_exit.read_text()
          and 'changes were not saved' not in exiting.stdout, 'Enter saves on exit')
    failed = menu(f'3\nretryword\n0\ny\n{folder}\n0\nn\n')
    check('Save target must be a file' in failed.stdout and failed.stdout.count('Save your changes?') == 2,
          'failed exit save keeps application and changes available')

    additional = pathlib.Path(folder) / 'additional words.txt'
    additional.write_text('mergedword 42\ncomputer 9999\n')
    more = menu(f'5\n9\n1\nCOMPUTER\n2\n{additional}\n3\n4\n5\nprefix merged\nback\n0\n0\nn\n')
    check('Word exists.' in more.stdout and 'Loaded 2 records' in more.stdout, 'more menu exact and merge')
    check('No searches recorded.' in more.stdout and 'Trie nodes:' in more.stdout, 'history and stats menu')
    check('frequency=42' in more.stdout and 'Goodbye.' in more.stdout, 'advanced commands return to menu')
    advanced_exit = menu('5\n5\nadd commandword\nquit\nc\n0\nn\n')
    check(advanced_exit.stdout.count('Save your changes?') == 2, 'advanced exit uses simple save protection')

    # Isolated default dictionary: never modify the user's real starter file.
    isolated = pathlib.Path(folder) / 'project with spaces'
    (isolated / 'data').mkdir(parents=True)
    (isolated / 'data' / 'dictionary.txt').write_text('initialword 7\n')
    def default_run(commands, *args):
        return subprocess.run([binary, *args], input=commands, text=True, capture_output=True,
                              timeout=15, cwd=isolated)
    initial = default_run('3\npersistedword\n0\n\n\n')
    persisted = isolated / 'data' / 'my_dictionary.txt'
    check(persisted.exists() and 'persistedword 1' in persisted.read_text(), 'default save path and Enter defaults')
    resumed = default_run('5\n1\npersistedword\n0\n0\n')
    check('my_dictionary.txt' in resumed.stdout and 'Word exists.' in resumed.stdout, 'automatic saved dictionary resume')
    forced_starter = default_run('5\n1\npersistedword\n0\n0\n', 'data/dictionary.txt')
    check('Word not found.' in forced_starter.stdout, 'explicit starter path overrides automatic resume')
    default_commands = default_run('exact persistedword\nquit\n', '--commands')
    check('Word exists.' in default_commands.stdout, 'command flag without explicit dictionary')
    default_benchmark = default_run('', '--benchmark', '1')
    check(default_benchmark.returncode == 0 and 'Parity: PASS' in default_benchmark.stdout,
          'benchmark flag without explicit dictionary')
    bad_modes = default_run('', '--commands', '--benchmark')
    check(bad_modes.returncode == 1 and 'Usage:' in bad_modes.stderr, 'conflicting startup modes rejected')
print(f'PASS: {checks} CLI integration checks.')
