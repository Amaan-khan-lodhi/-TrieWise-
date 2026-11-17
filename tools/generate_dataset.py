#!/usr/bin/env python3
"""Generate reproducible SYNTHETIC strings for scaling experiments, not English words."""
import argparse
import pathlib
import random

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--count', type=int, default=10000)
parser.add_argument('--seed', type=int, default=20261016)
parser.add_argument('--output', type=pathlib.Path, default=pathlib.Path('data/synthetic10000.txt'))
args = parser.parse_args()
if not 1 <= args.count <= 1000000:
    parser.error('--count must be between 1 and 1000000')
if args.output.exists():
    parser.error('output already exists; choose a new filename')
rng = random.Random(args.seed)
words = {'computer', 'computing', 'compile', 'algorithm', 'network', 'security', 'data', 'database'}
if args.count < len(words):
    words = set(sorted(words)[:args.count])
while len(words) < args.count:
    words.add(''.join(rng.choice('abcdefghijklmnopqrstuvwxyz') for _ in range(rng.randint(4, 14))))
args.output.parent.mkdir(parents=True, exist_ok=True)
with args.output.open('w', encoding='ascii', newline='\n') as output:
    output.write(f'# Synthetic benchmark strings, count={args.count}, seed={args.seed}\n')
    output.write('# Frequencies are invented demonstration weights, not corpus statistics.\n')
    for word in sorted(words):
        output.write(f'{word} {rng.randint(1, 10000)}\n')
print(f'Wrote {len(words)} synthetic entries to {args.output}')
