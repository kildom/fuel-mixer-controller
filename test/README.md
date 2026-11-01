# Host tests for controller logic

Run all tests from this directory:

```sh
make test
```

Each `test_*.cpp` file defines a `void test()` function. The runtime starts with
the application defaults and inactive inputs. Use `param(id, value)` to change
a parameter, `on(input)` / `off(input)` to set an input, and `delay(seconds)`
to advance simulated time. Delays are rounded to the nearest 100 ms; each
100 ms step calls `procesuj()` and records output changes. `time()` returns the
current simulation time in seconds. `jump(seconds)` advances to an absolute
simulation time in seconds (at 100 ms resolution), processing each elapsed
step; it rejects targets earlier than the current time.

For each test, `make test` writes a VCD and a matching GTKWave save file under
`results/` (for example, `test_loader.vcd` and `test_loader.gtkw`). Open the
`.gtkw` file in GTKWave to load the VCD with every signal already in the
waveform view. VCD time units are 100 ms; signals are declared in this order:
inputs, outputs, parameters.

Zephyr kernel and logging APIs used by `logic.c` are stubbed under `mock/`.
