simple agent
--------------

A simple AI agent in c++

## Features

- Chat history
- OpenAI-compatible `tools` support with built-in `read_file`

## Requirements

To build this project, system MUST have following libraries:

- libcurl

## Build

```bash
mkdir -p build && cd $_
cmake ..
make -j30
```

## Usage

```bash
API_KEY="$OPENROUTER_API_KEY" API_URL="https://openrouter.ai/api/v1" MODEL="openrouter/free" ./build/src/simple_agent
```

### Ollama (OpenAI-compatible API)

```bash
API_KEY="ollama" API_URL="http://localhost:11434/v1" MODEL="gemma4:e4b" ./build/src/simple_agent
```

### LM Studio (OpenAI-compatible API)

```bash
API_KEY="lmstudio" API_URL="http://localhost:1234/v1" MODEL="gemma4:e4b" ./build/src/simple_agent
```

## clang-format

To run clang-format, use following command

```bash
cmake --build build/ --target format
```
