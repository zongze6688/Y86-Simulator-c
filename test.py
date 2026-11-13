import argparse
import difflib
import json
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bin', required=True, help='simulator command')
    parser.add_argument('--save_mid', action='store_true', help='save outputs in temp_answer/')
    args = parser.parse_args()
    try:
        command = shlex.split(args.bin)
        if not command:
            raise ValueError('empty simulator command')
    except ValueError as error:
        parser.error(str(error))

    answers = sorted(Path('answer').glob('*.json'))
    if not answers:
        print("Error: No test cases found in 'answer/' directory.", file=sys.stderr)
        return 1

    passed = 0
    first_diff = False
    with tempfile.TemporaryDirectory() as temporary:
        output_dir = Path('temp_answer') if args.save_mid else Path(temporary)
        output_dir.mkdir(exist_ok=True)
        for answer_path in answers:
            name = answer_path.stem
            input_path = Path('test') / f'{name}.yo'
            output_path = output_dir / answer_path.name
            error = None
            actual = expected = None
            try:
                with answer_path.open(encoding='utf-8') as source:
                    expected = json.load(source)
                # Truncate first so a missing input cannot leave an old result.
                with output_path.open('w', encoding='utf-8') as output:
                    with input_path.open('rb') as source:
                        result = subprocess.run(command, stdin=source, stdout=output,
                                                stderr=subprocess.PIPE, timeout=2)
                if result.returncode:
                    error = f'exit code {result.returncode}'
                    details = result.stderr.decode('utf-8', errors='replace').strip()
                    if details:
                        error += f': {details[:500]}'
                else:
                    with output_path.open(encoding='utf-8') as source:
                        actual = json.load(source)
            except subprocess.TimeoutExpired:
                error = 'execution timed out after 2 seconds'
            except (OSError, ValueError) as exc:
                error = str(exc)

            if error is None and actual == expected:
                print(f'[PASS] {answer_path.name}')
                passed += 1
                continue
            print(f'[FAIL] {answer_path.name}' + (f' ({error})' if error else ''))
            if not first_diff:
                if error is None:
                    actual_text = json.dumps(actual, indent=2, sort_keys=True).splitlines()
                    expected_text = json.dumps(expected, indent=2, sort_keys=True).splitlines()
                    print('\n'.join(difflib.unified_diff(actual_text, expected_text,
                                                        fromfile='actual', tofile='expected', lineterm='')))
                first_diff = True

    print('\n' + '=' * 40)
    print(f'Test Cases Passed: {passed} / {len(answers)}')
    print(f'Base Score:        {passed / len(answers) * 60:.2f} / 60.00')
    print('=' * 40)
    if passed == len(answers):
        print('All correct! Congratulations!')
        return 0
    print(f'::error::Only passed {passed} out of {len(answers)} tests.')
    return 1


if __name__ == '__main__':
    sys.exit(main())
