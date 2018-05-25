## Drain

Build release:

```sh
  $ mkdir BD_release && cd BD_release && cmake -DCMAKE_BUILD_TYPE=Release .. && make
```

Build debug:

```sh
  $ mkdir BD_debug && cd BD_debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
```

Run tests:

```sh
  $ make test
```
