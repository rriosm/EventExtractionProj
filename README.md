# CENTERIS

A modular C++ / Qt framework and application suite designed for video event extraction, speech-to-text processing, and multimodal media analysis. The repository is organized as a multi-project Qt workspace consisting of core dynamic shared libraries and a CLI processing tool.

---

## Workspace Structure

The project is structured into shared libraries and executable applications:

```text
CENTERIS/
├── Logger/                 # Core cross-platform dynamic logging library
│   ├── inc/                # Public headers (Logger.h, Logger_global.h)
│   ├── src/                # Implementation files
│   └── Logger.pro          # Library build configuration
├── Utils/                  # General utility library (networking, serialization, MIME)
│   ├── inc/                # Public headers (Utils.h, DownloadManager.h, Url.h, etc.)
│   ├── src/                # Implementation files
│   └── Utils.pro           # Library build configuration
├── EventExtractor/         # Main consumer CLI pipeline & experiment tool
│   ├── inc/                # Application headers
│   ├── src/                # Pipeline logic and main.cpp
│   └── EventExtractor.pro  # Application build configuration
└── CENTERIS.pro            # Top-level subdirs build definition
```
---

### AI Prompts

```text
CENTERIS
├── Resources/
│   ├── Prompts/
│   │   ├── BaselinePrompt.txt
│   │   ├── SystemProfile.txt
│   │   ├── UserPromptTemplate.txt
```

## Module Overview

### 1. `Logger`

* **Type:** Shared Library (`TEMPLATE = lib`)
* **Role:** Lightweight, configurable logging framework built on top of Qt's messaging architecture.
* **Features:**
* Export/import ABI macros (`LOGGER_EXPORT`).
* Suppression of debug/info logging in release builds (`QT_NO_DEBUG_OUTPUT`).
* Structured build staging and `pkg-config` metadata export.
* Relocatable macOS dylib configurations utilizing `@rpath`.

### 2. `Utils`

* **Type:** Shared Library (`TEMPLATE = lib`)
* **Role:** Core abstraction layer and utilities supporting:
* Network download orchestration via `DownloadManager`.
* URL processing, MIME-type lookups, and unique ID generators.
* Binary stream serialization helpers (`Serialize_e`).
* Direct dependency linkage against `Logger`.

### 3. `EventExtractor`

* **Type:** Application Executable (`TEMPLATE = app`)
* **Role:** Multimodal media processor, pipeline orchestrator, and experimental evaluation CLI for speech transcription, video segmentation, and multimodal event spotting.

---

## Requirements & Toolchain

* **Qt:** Qt 5.12 LTS through Qt 5.15 LTS (or Qt 6)
* **Modules:** `core`, `network`, `sql`, `xml`, `xmlpatterns` (GUI excluded: `QT -= gui`)
* **Compiler:**
* **macOS:** Apple Clang / Xcode Command Line Tools (macOS 10.15 SDK deployment target supported)
* **Linux:** GCC 7+ or Clang 8+
* **Windows:** MSVC 2017+ or MinGW
* **C++ Standard:** C++17 (or C++11 minimum)
* **Build System:** `qmake` + `make`

---

## Build Order & Compilation

### Option 1: Top-Level Build

From the project root:

```bash
# 1. Run qmake on the root multi-project file
qmake CENTERIS.pro

# 2. Compile all targets in sequence
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

### Option 2: Step-by-Step Compilation

#### 1. Build and Stage `Logger`

```bash
cd Logger
qmake Logger.pro CONFIG+=release
make
make install
cd ..
```

#### 2. Build `Utils`

```bash
cd Utils
qmake Utils.pro CONFIG+=release
make
cd ..
```

#### 3. Build `EventExtractor`

```bash
cd EventExtractor
qmake EventExtractor.pro CONFIG+=release
make
cd ..
```


---

## Output Directories

Build artifacts are separated from intermediate object files:

* **Object & Intermediate Files:** Staged in `.obj/`, `.moc/`, `.rcc/`, and `.ui/` under each project's build directory.
* **Compiled Libraries:** Output to `../lib/Debug` or `../lib/Release`.
* **Binary Naming:** Debug library artifacts receive the `-debug` suffix (e.g., `libLogger-debug.dylib` on macOS or `libLogger-debug.so` on Linux).

---

## CLI Usage & Commands

The `EventExtractor` binary supports full-pipeline video analysis, standalone event extraction, and various evaluation experiments.

### Command Reference Flag Summary

| Flag | Parameter Description |
| --- | --- |
| `-m` | Execution mode (`pipeline`, `evtsExtraction_only`, `experiment`) |
| `-exp` | Specific experiment profile name |
| `-i` | Input video file path |
| `-td` | Target output directory |
| `-af` | Extracted audio format (e.g., `mp3`) |
| `-s2t` | Speech-to-text model identifier |
| `-convType` | Transcript converter format (e.g., `DeepgramWebVttConverter`) |
| `-vsm` | Video segmentation mode profile |
| `-cd` | Segment/clip duration (seconds) |
| `-aim` | Vision/multimodal model identifier (e.g., `gemini-2.5-flash`) |
| `-sf` | Target slowdown factors as an array string |
| `-ta` | Task analysis profile |
| `-sp` | System instruction/role profile |
| `-st` | Searched target event terms |
| `-sType` | Term search operator (`any`, `all`) |
| `-ct` | Detection confidence threshold (`0.0` - `1.0`) |
| `-sm` | Path to synchronized manifests JSON |
| `-gmd` | Path to governed markdown output results |
| `-bmd` | Path to baseline markdown output results |
| `-gt` | Path to ground truth CSV annotations |

---

### 1. Complete Pipeline

Executes end-to-end ingestion: audio extraction, transcription, video segmentation, temporal alignment, and event extraction.

```bash
./EventExtractor -m <mode> \
  -i "<video_file>" \
  -af <audio_format> \
  -s2t <audio_speech_2_text_model> \
  -convType <converter_type>
  -vsm <video_seg_mode>
  -cd <clip_duration>
  -aim <ai_model_type>
  -sf <slowdown_factors>
  -ta <task_analisis>
  -sp <system_profile>
  -st <searched_terms>
  -sType <search_type>
  -ct <confidence_threshold>
  -td "<target_dir>"
```

**Example:**

```bash
./EventExtractor -m pipeline \
  -i "/path/to/match_footage.mp4" \
  -af mp3 \
  -s2t DeepgramS2TNova2GeneralEN \
  -convType DeepgramWebVttConverter \
  -vsm Gemini25_Flash5fps21crf \
  -cd 300 \
  -aim gemini-2.5-flash \
  -sf "['6.00', '8.00', '4.00', '2.00', '0.00']" \
  -ta video_analysis_task \
  -sp fifa_referee \
  -st "['goal', 'foul', 'corner', 'red card', 'yellow card']" \
  -sType any \
  -ct 0.80 \
  -td "/output/pipeline_run"
```
---

### 2. Events Extraction Module (Standalone)

Extracts events on pre-segmented and pre-synchronized video/audio streams using an existing synchronized manifest file.

```bash
./EventExtractor -m <mode> \
  -sm "<synced_manifests>" \
  -aim <ai_model_type> \
  -sf "['<slowdown_factor>']" \
  -cd <clip_duration> \
  -ta <task_analisis> \
  -sp <system_profile> \
  -st <searched_terms> \
  -sType <search_type> \
  -ct <confidence_threshold> \
  -td "<target_dir>"
```
**Example:**

```bash
./EventExtractor -m evtsExtraction_only \
  -sm "/path/to/synchronized_manifests.json" \
  -aim gemini-2.5-flash \
  -sf "['6.00']" \
  -cd 300 \
  -ta video_analysis_task \
  -sp fifa_referee \
  -st "['goal', 'foul', 'throw-in', 'corner', 'red card', 'yellow card']" \
  -sType any \
  -ct 0.80 \
  -td "/output/events_extraction"
```
---

### 3. Experiments & Evaluation

#### Token Validation Experiment

Validates structural integrity and token density between annotations and WebVTT parent files.

```bash
./EventExtractor -m experiment \
  -exp token_validation_experiment \
  -td "<path_to_annotation_and_vtt_parent_dir>"
```


#### Baseline vs. Governed Generation Experiment

Generates unconstrained baseline outputs vs. governed outputs enforced by the manifest definitions.

```bash
./EventExtractor -m experiment \
  -exp baseline_governed_experiment \
  -aim gemini-2.5-flash \
  -st "['goal', 'foul', 'throw-in', 'corner', 'red card', 'yellow card']" \
  -sType any \
  -ct 0.80 \
  -sm "/path/to/synced_manifests.json" \
  -td "/output/baseline_governed_run"
```

#### Audit Governance

Compares baseline vs. governed Markdown artifacts to evaluate hallucination reduction and constraint enforcement.

```bash
./EventExtractor -m experiment \
  -exp governance_audit \
  -aim gemini-2.5-flash \
  -gmd "/path/to/governed_results.md" \
  -bmd "/path/to/baseline_results.md" \
  -td "/output/governance_audit"
```

#### Report Events Found

Parses governed Markdown reports to extract and summarize event detection distributions and metrics.

```bash
./EventExtractor -m experiment \
  -exp report_events_found \
  -gmd "/path/to/governed_results.md" \
  -td "/output/event_reports"
```

#### Precise Event Spotting & Clock-Domain Alignment

Calculates temporal offset accuracy and alignment against ground-truth CSV timestamps.

```bash
./EventExtractor -m experiment \
  -exp event_spotting_domain_alignment \
  -aim gemini-2.5-flash \
  -gt "/path/to/ground_truth.csv" \
  -gmd "/output/20260911-22h50min-Slowdown_6x/gemini-2.5-flash-6.00x_Slowdown-SystemProfile_UserPrompt.md" \
  -td "/output/domain_alignment"
```

---

## License

Copyright 2026

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

[http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)


Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
