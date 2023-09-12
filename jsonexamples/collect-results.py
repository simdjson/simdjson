from itertools import groupby
from pathlib import Path
from typing import Any, Iterable, Literal, Optional, OrderedDict, cast
from io import BufferedWriter, BufferedReader
import re
import sys

ContainerState = Literal['orig', 'array', 'flat']
ScalarState = Literal['orig', '1digit', 'str']
StringState = Literal['orig', 'unescaped', 'ascii', 'empty']
CONTAINER_STATES: list[ContainerState] = ['orig', 'array', 'flat']
SCALAR_STATES: list[ScalarState] = ['orig', '1digit', 'str']
STRING_STATES: list[StringState] = ['orig', 'unescaped', 'ascii', 'empty']

class Result:
    class Metric:
        def __init__(self, name: str, value: float, units: str):
            self.name = name
            self.value = value
            self.units = units
        
        def best(self, other: 'Result.Metric'):
            assert self.name == other.name
            assert self.units == other.units
            if self.name == 'Speed':
                return self if self.value > other.value else other
            else:
                return self if self.value < other.value else other

    class Stage:
        def __init__(self, stage: str):
            self.stage = stage
            self.metrics: dict[str, Result.Metric] = {}

        @property
        def speed(self):
            return self.metrics['Speed']

        @property
        def cycles(self):
            return self.metrics['Cycles']

        @property
        def instructions(self):
            return self.metrics['Instructions']

        @property
        def misses(self):
            return self.metrics['Misses']

        def _merge_in(self, other: 'Result.Stage'):
            assert self.stage == other.stage
            for metric in other.metrics.values():
                if metric.name not in self.metrics:
                    self.metrics[metric.name] = metric
                self.metrics[metric.name] = self.metrics[metric.name].best(metric)

    def __init__(self, json_file: Path):
        self.json_file = json_file
        self.stages = dict[str, Result.Stage]()
        match = re.match(r'(.*)-([^-]*)-([^-]*)-([^-]*)$', json_file.stem)
        if match:
            self.base_json_file = match.group(1)
            assert match.group(2) in CONTAINER_STATES
            self.container_state: ContainerState = cast(ContainerState, match.group(2))
            assert match.group(3) in SCALAR_STATES
            self.scalar_state: ScalarState = cast(ScalarState, match.group(3))
            assert match.group(4) in STRING_STATES
            self.string_state: StringState = cast(StringState, match.group(4))
        else:
            self.base_json_file = self.json_file.stem
            self.container_state = 'orig'
            self.scalar_state = 'orig'
            self.string_state = 'orig'
        self.docs_per_second: float = -1

    def __lt__(self, other: 'Result'):
        if self.base_json_file != other.base_json_file:
            return self.base_json_file < other.base_json_file
        if self.container_state != other.container_state:
            return CONTAINER_STATES.index(self.container_state) < CONTAINER_STATES.index(other.container_state)
        if self.scalar_state != other.scalar_state:
            return SCALAR_STATES.index(self.scalar_state) < SCALAR_STATES.index(other.scalar_state)
        if self.string_state != other.string_state:
            return STRING_STATES.index(self.string_state) < STRING_STATES.index(other.string_state)
        return False
            
    def merge(self, other: 'Result'):
        merged = Result(self.json_file)
        merged._merge_in(self)
        merged._merge_in(other)
        return merged

    def _merge_in(self, other: 'Result'):
        assert self.json_file == other.json_file
        if other.docs_per_second > self.docs_per_second:
            self.docs_per_second = other.docs_per_second
        for stage in other.stages.values():
            if stage.stage not in self.stages:
                self.stages[stage.stage] = Result.Stage(stage.stage)
            self.stages[stage.stage]._merge_in(stage)

    @property
    def stage1(self):
        return self.stages['Stage 1']

    @property
    def stage2(self):
        return self.stages['Stage 2']

def read_results(results_file: Path):
    with open(result_file, 'rt') as input:
        result = None
        stage = None

        prev_line = None
        for line in input:
            if re.match(r'=+$', line):
                assert result is None
                assert prev_line is not None
                result = Result(Path(prev_line.strip()))

            # Stage
            match = re.match(r'\|-(.+)$', line)
            if match:
                assert result is not None
                stage = Result.Stage(match.group(1).strip())
                result.stages[stage.stage] = stage

            # Metrics
            if stage is not None:
                match = re.match(r'\|([^:]+):\s*([-+0-9.]+)\s+([^(-]+)', line)
                if match and stage is not None:
                    metric = Result.Metric(match.group(1).strip(), float(match.group(2)), match.group(3).strip())
                    stage.metrics[metric.name] = metric

            # Documents per second
            match = re.match(r'\s*([-+0-9.]+)\s*documents parsed per second', line)
            if match:
                assert result is not None
                result.docs_per_second = float(match.group(1))
                yield result
                result = None
                stage = None
            
            prev_line = line

# Merge multiple results for the same file
all_results = dict[Path, Result]()
for result_file in sys.argv[1:]:
    for result in read_results(Path(result_file)):
        if result.json_file in all_results:
            all_results[result.json_file] = all_results[result.json_file].merge(result)
        else:
            all_results[result.json_file] = result

def print_row(row: Iterable, key_lengths: OrderedDict[str, int], rjust: set[str]):
    column_iter = iter(row)
    for (key, width) in key_lengths.items():
        value = str(next(column_iter))
        if key in rjust:
            print(f"| {str(value).rjust(width)} ", end='')
        else:
            print(f"| {str(value).ljust(width)} ", end='')
    print("|")

def print_table(rows: list[OrderedDict], key_lengths: Optional[OrderedDict[str, int]] = None, rjust = set[str]()):
    if key_lengths is None:
        key_lengths = OrderedDict[str, int]()
        for entry in rows:
            for (key, value) in entry.items():
                if key not in key_lengths:
                    key_lengths[key] = len(key)
                key_lengths[key] = max(key_lengths[key], len(str(value)))

    print_row(list(key_lengths.keys()), key_lengths, rjust)
    print("|", *[ f"{''.ljust(len+2, '-')}|" for len in key_lengths.values() ], sep='')

    for row in rows:
        print_row(row.values(), key_lengths, rjust)

for (file, results) in groupby(sorted(all_results.values()), lambda r: r.base_json_file):
    results = [*results]
    print()
    print(f"# {file}.json Branch Miss Variants")
    print()
    print_table(
        [
            OrderedDict([
                ('Contain',  result.container_state),
                ('Scalars',  result.scalar_state),
                ('Strings',  result.string_state),
                ('Cycles',   '%.4f' % result.stage2.cycles.value),
                ('Instrs',   '%.4f' % result.stage2.instructions.value),
                ('Misses',   int(result.stage2.misses.value)),
                ('Docs/sec', '%.1f' % result.docs_per_second),
            ])
            for result in results
        ],
        rjust = set(['Cycles', 'Instrs', 'Misses', 'Docs/sec'])
    )
    misses = {
        (r.container_state, r.scalar_state, r.string_state): int(r.stage2.misses.value)
        for r in results
    }

    print()
    print('## Container State Transition Miss Reduction')
    print()
    rows = list[OrderedDict[str, object]]()
    PRINT_STRING_STATES: list[StringState] = [s for s in STRING_STATES if s != 'unescaped']
    for (i,from_state) in enumerate(CONTAINER_STATES[0:-1]):
        to_state = CONTAINER_STATES[i+1]
        rows.append(OrderedDict([
            ('Contain', f"{from_state} -> {to_state}"),
            *[
                (
                    f"{scalar_state} {string_state}",
                    misses[(from_state, scalar_state, string_state)] - misses[(to_state, scalar_state, string_state)]
                )
                for scalar_state in SCALAR_STATES
                for string_state in PRINT_STRING_STATES
            ]
        ]))
    print_table(rows, rjust = [*rows[0].keys()][1:])

    print()
    print('## Scalar State Transition Miss Reduction')
    print()
    rows = list[OrderedDict[str, object]]()
    for (i,from_state) in enumerate(SCALAR_STATES[0:-1]):
        to_state = SCALAR_STATES[i+1]
        rows.append(OrderedDict([
            ('Scalars', f"{from_state} -> {to_state}"),
            *[
                (
                    f"{container_state} {string_state}",
                    misses[(container_state, from_state, string_state)] - misses[(container_state, to_state, string_state)]
                )
                for container_state in CONTAINER_STATES
                for string_state in PRINT_STRING_STATES
            ]
        ]))
    print_table(rows, rjust = [*rows[0].keys()][1:])

    print()
    print('## String State Transition Miss Reduction')
    print()
    rows = list[OrderedDict[str, object]]()
    for (i,from_state) in enumerate(PRINT_STRING_STATES[0:-1]):
        to_state = PRINT_STRING_STATES[i+1]
        rows.append(OrderedDict([
            ('Strings', f"{from_state} -> {to_state}"),
            *[
                (
                    f"{container_state} {scalar_state}",
                    misses[(container_state, scalar_state, from_state)] - misses[(container_state, scalar_state, to_state)]
                )
                for container_state in CONTAINER_STATES
                for scalar_state in SCALAR_STATES
            ]
        ]))
    print_table(rows, rjust = [*rows[0].keys()][1:])
