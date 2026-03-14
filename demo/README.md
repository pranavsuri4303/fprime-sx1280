# fprime-sx1280 Demos

This directory contains F´ demo deployments that exercise the `Sx1280Radio` library.

## Python/uv environment

- Python version: 3.11 (see `.python-version`)
- Managed via [uv](https://github.com/astral-sh/uv) using `pyproject.toml` in this directory.

## Setting up the environment

From the repo root:

```bash
cd demo
uv venv --python 3.11
uv sync
```

This will create `demo/.venv` and install `fprime-tools`.

## Creating the LoRa CCSDS demo deployment

From the repo root, after setting up the env:

```bash
cd demo
source .venv/bin/activate
cd lora_ccsds_demo
fprime-util new
```

If `fprime-util new` initializes a `.git` here, remove it so that the root repo remains the only git repository:

```bash
rm -rf .git
```

Then configure the deployment to use the parent `Sx1280Radio` library (via `library.cmake`) and wire `Sx1280Radio.LoRaRadioAdapter` into `ComCcsds.FramingSubtopology`.

## Building and running the demo

From the repo root:

```bash
cd demo
source .venv/bin/activate
cd lora_ccsds_demo
fprime-util generate
fprime-util build
fprime-gds
```

The demo deployment should then send and receive F´ traffic over the LoRa link via the `LoRaRadioAdapter` component.
