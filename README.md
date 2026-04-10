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

## Price information

If your llm provider not free, ex: Input Price $0.429, Output Price $4.41, you can setup `INPUT_PRICE` and `OUTPUT_PRICE` variable to display cost.

```bash
INPUT_PRICE=0.429 OUTPUT_PRICE=4.41 API_KEY="$OPENROUTER_API_KEY" API_URL="https://openrouter.ai/api/v1" MODEL="z-ai/glm-5.1" ./build/src/simple_agent
```

How can i know the price? Take openrouter for example, we can use following command to get model price  (or via: https://openrouter.ai/z-ai/glm-5.1/pricing)

```bash
curl https://openrouter.ai/api/v1/models   -H "Authorization: Bearer $OPENROUTER_API_KEY"
# lots of json data, we only show glm-5.1 here
```

And you will get:

```json
 {
      "id": "z-ai/glm-5.1",
      "canonical_slug": "z-ai/glm-5.1-20260406",
      "hugging_face_id": "zai-org/GLM-5.1",
      "name": "Z.ai: GLM 5.1",
      "created": 1775578025,
      "description": "GLM-5.1 delivers a major leap in coding capability, with particularly significant gains in handling long-horizon tasks. Unlike previous models built around minute-level interactions, GLM-5.1 can work independently and continuously on...",
      "context_length": 202752,
      "architecture": {
        "modality": "text->text",
        "input_modalities": [ ... ],
        "output_modalities": [ ... ],
        "tokenizer": "Other",
        "instruct_type": null
      },
      "pricing": {
        "prompt": "0.00000126",
        "completion": "0.00000396"
      },
      "top_provider": {... },
      "per_request_limits": null,
      "supported_parameters": [...],
      "default_parameters": {...},
      "knowledge_cutoff": null,
      "expiration_date": null,
      "links": {
        "details": "/api/v1/models/z-ai/glm-5.1-20260406/endpoints"
      }
## clang-format

To run clang-format, use following command

```bash
cmake --build build/ --target format
```

## References

- [Learn Claude Code](https://learn.shareai.run/en/)
- [Build your own agent in Rust](https://odysa.github.io/mini-claw-code/en/)
