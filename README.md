# BatchSchedulingSolver

Research code for batch scheduling problems.

## Compilation

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel && cmake --install build --config Release --prefix install
```

Setup Python environment to use the Python scripts:
```shell
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Download data files:
```shell
python scripts/download_data.py
```

## Implemented algorithms
