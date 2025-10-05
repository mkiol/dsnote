# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

- More documentation: [CONTRIBUTING.md](./CONTRIBUTING.md) and CHANGELOG.md.

### Changed

- Upgrade from Qt5 to Qt6!

## [4.8.3] - 2025-08-15

### Fixed in 4.8.3

- Support for older CPUs in OpenBLAS
- Coqui XTTS license download error
- Don't strip RHVoice STT libraries

### Changed in 4.8.3

- SFOS: Strip binaries
- SFOS UI: Move copy button to left side
- Update README

## [4.8.2] - 2025-07-13

### Fixed in 4.8.2

- Reading doesn't stop on TTS error
- Crash when TTS engine gives corrupted audio file
- Invalid metainfo file

### Added in 4.8.2

- New languages in Whisper: Azerbaijani, Belarusian, Malayalam, Tamil, Kannada
- New Piper voices: Spanish, Hindi, Malayalam, Nepali

### Changed in 4.8.2

- Flatpak: Downgrade numba to 0.60.0

## [4.8.1] - 2025-07-10

### Fixed in 4.8.1

- 'Configure GKS' doesn't work on KDE Plasma
- Outdated version
- Translator models download error
- XDO build error
- RHVoice lib broken on SFOS aarch64

### Added in 4.8.1

- Coqui MMS voices for: Kannada, Malayalam, Tamil
- German translation
- New Vosk model for German: tuda-de
- New translator models
- Info about required disk space during install
- Options to disable torch and CT2 detection

### Changed in 4.8.1

- Improve libwhisper build script
- Update translator models
- Clean-up leftover directories in Flatpak manifest
- Make XDO patch more readable

## [4.8.0] - 2025-06-20

### Added in 4.8.0

- Option for setting window size
- CMD option for exporting TTS audio to a file
- "Case sensitive" option in rules
- Beep sounds and option to play beep when listening
- Option to pause listening while STT processing
- FUTO Whisper models
- KBLab Swedish Whisper models
- Kokoro TTS engine and model
- F5-TTS engine and model for English and Chinese
- Parler-TTS engine and models
- Arabic to English translator model
- Turkish translation
- Catalan translations
- Arabic translation
- SAM TTS engine
- Text transformation with "rules"
- Echo mode
- Action for reading text from CMD interface

### Fixed in 4.8.0

- Invalid rpath in RHVoice and libstt on SFOS
- Build error on SFOS
- CUDA build error on Arch
- Build error when compiling with GCC-15
- XDO build failed
- Invalid toast text color
- Infinite loop while regexp rule processing
- FasterWhisper GPU memory not released
- Stop listening tone played twice
- Dark text on dark background in toast notification
- Invalid Kokoro initialization

### Changed in 4.8.0

- Enable speed control for XDO
- Do not add extension to export filename
- Bind X11 shortcuts also on portal session error
- Clear pending actions queue on cancel
- Increase beam size in STT 'best performance' profile
- Update Flatpak runtime to 5.15-24.08
- Improve global shortcuts configuration
- Limit Python scan on Flatpak to speed up start
- Group WhisperSpeech models into pack
- Speed up cancelling in Coqui, F5 and Kokoro TTS
- Improve STT models packs
- Rename "languages" menu to "languages & models"
- Show auto Whisper models in pack
- Show STT models in packs
- Use existing note as initial context in STT decoding
- Enable Python support in ARM and tiny packages
- By default enable GPU for TTS engines
- Update RHVoice voices for Czech language
- Add RHVoice Mateo TTS voice
- Use astrunc splitter for Chinese, Japanese, Hindi TTS models
- Update translator models
- Install default audio samples for voice cloning
- Show 'experimental' icon on some models
- Show model card URL in model's info
- Improve UI when using Arabic language
- Add option to disable audio normalization
- Support UTF in YDO
- Improve YDO integration
- Use custom logger
- Update html2md to v1.6.4
- Update fmtlib to v11.1.3
- Add BUILD_XKBCOMMON cmake option

## [4.7.1] - 2025-01-27

### Fixed in 4.7.1

- Crash when CPU doesn't have AVX
- Invalid task icon state when not configured

### Added in 4.7.1

- New translator models
- Spanish translation
- French-Canadian translation

### Changed in 4.7.1

- UI: Expose keystroke delay option
- Enable Spanish translation

## [4.7.0] - 2025-01-20

### Added in 4.7.0

- Text transformation with "rules"
- Echo mode
- Action for reading text from CMD interface
- TTS WhisperSpeech 1.95 8lang small model
- New command line options and DBus API
- Vulkan support for whisper.cpp engine
- Engine profile setting option
- Whisper-large-v3-turbo model
- Experimental ZLUDA support in whisper-cpp
- Support for 'insert into active window' in Wayland
- SAM TTS engine
- Crisper-Whisper model
- Option to insert new text at the cursor position
- Slovenian translation
- New Latvian Piper voice
- Option for STT stats
- Support for silence and speed control tags
- Norwegian translation
- Audio context size engine option
- OpenVINO support in whisper-cpp engine

### Fixed in 4.7.0

- Translator models missing on SFOS
- Echo mode didn't work for voice cloning models
- Build error on SFOS
- DBus call doesn't stop in not configured mode
- Logger error
- Invalid Vulkan ID
- Wrong Vulkan device is selected
- Whisper.cpp fails if multiple Vulkan devices found
- Crash when using WhisperSpeech
- Invalid torch version in Flatpak package
- Runtime error when using hipblas whisper.cpp
- Missing dependencies for WhisperSpeech
- Parallel build error when generating translation files
- Build error with ffmpeg >= 61

### Changed in 4.7.0

- Make XDO keystrokes sending method default
- Use libxdo for keystrokes sending
- Improve DBus API
- Make --print-* CMD options more granular
- Update translation for Ukrainian and Russian
- Refactor: improve settings window
- Improve "missing add-on" warning message
- Disable echo when transcribing file
- Enable Vulkan iGPU in whisper.cpp by default
- Add more shortcuts and labels to UI buttons
- By default don't use Vulkan iGPU
- Use system tray icon animation to indicate status
- Add scan button for keyboard shortcuts
- Improve 'insert at the cursor position'
- Add new text appending mode: replace
- Show warning when Flatpak addon is incompatible
- Enable whisper.cpp Vulkan on ARM
- Show Flatpak addon installing instruction
- Update CUDNN to version 9.5.1
- Make sequential install of whisper.cpp libs
- Update torch to version 2.5.1
- ROCm update to version 6.2.2
- Update whisper.cpp to version 1.7.1
- Update FFmpeg to version 7.1
- Handle global shortcuts via XDG desktop portal

## [4.6.1] - 2024-08-23

### Fixed in 4.6.1

- Disabled TTS GPU acceleration on ROCm
- Build error
- Crash when BMI2 is not supported in CPU

### Added in 4.6.1

- Fixes for ZLUDA
- Support for x86free keys in global key shortcuts

### Changed in 4.6.1

- Update translator models
- Don't require AVX2 and FMA for whisper-cpp on GPU
- Swedish translation update
- Disable 'flash attention' by default

## [4.6.0] - 2024-08-16

### Added in 4.6.0

- Auto language detection for Whisper STT
- New translator models
- Danish-to-English translator model
- New RHVoice model for Slovak
- New Piper voices for: Welsh, Italian, English
- Option for STT stats
- Support for silence and speed control tags
- Norwegian translation
- Audio context size engine option
- OpenVINO support in whisper-cpp engine
- Separate GPU options for engines

### Fixed in 4.6.0

- Build error on SFOS arm32
- STT stats option can't be disabled
- Crash when using whisper-cpp with ROCm
- Up and down keys don't work in text editor
- Translate button is enabled when no models
- Crash in WhisperSpeech engine
- ARM build error
- Minor UI glitches
- Missing addons warning is always visible
- Flatpak GPU addon build error
- Can't set Python path

### Changed in 4.6.0

- Remove 'beta' label
- Improve STT stats tag removal
- Don't insert STT stats when using action
- Don't read and translate STT stats
- Update RHVoice and enable Croatian voice
- Don't require AVX for translator
- Use tiny model for language auto-detection
- Disable manual file export path on Flatpak
- Don't download OpenVINO model on ARM
- SFOS: Enable 'translate to English' for Whisper
- SFOS: Don't show subtitle format when exporting
- Improve STT with whisper-cpp
- Switch languages when input and output languages are the same
- Improve translator UI
- Show multi-voice models in groups
- Update OpenBLAS to v0.3.27
- Update ctranslate2 and RHVoice
- Enable flash attention by default
- Don't translate control tags
- Add context menu for inserting control tags
- Add 'read all' context option
- Fallback to libwhisper.so when other libs missing
- Retry if flash-att isn't supported in faster-whisper
- Disable OpenVINO for GPU by default
- Don't show warning when NVIDIA/AMD GPU is not found
- Enable whisper-cpp hipblas and clblast by default
- Expose Whisper options in the settings
- Use separate GPU options for engines
- Don't crash on PulseAudio init error
- Re-enable Spanish Piper MLS voices
- Update whisper.cpp

## [4.5.0] - 2024-06-29

### Added in 4.5.0

- Subtitles generation with STT
- Option to mix speech with audio from existing file
- Open subtitles embedded in video file
- Support for multiple audio streams in video file
- Auto switch to plain text format when note is empty
- Option to enable/disable text in notifications
- Speed control for XTTS
- 'Repair text' option with punctuation restoration
- Option to define X11 compose file
- Option to set font in text editor
- Translator buttons for switching between languages
- Show/hide advanced settings
- Tips in UI
- 'Sync subtitles' setting option
- 'File import action' option
- Show/hide option to tray menu
- Option to remember the last note
- Scroll to beginning/bottom buttons (SFOS)
- Custom model support
- Russian translation
- Ukrainian translation
- Swedish translation
- Italian translation
- Polish translation
- Czech translation
- Lithuanian translation

### Features in 4.5.0

- Speech-to-Text (STT) with multiple engines: Whisper, Faster-Whisper, Whisper.cpp, Vosk, April, DeepSpeech
- Text-to-Speech (TTS) with multiple engines: Piper, RHVoice, Mimic3, Coqui XTTS, WhisperSpeech
- Translation support with Bergamot translator
- Voice cloning capabilities
- Global keyboard shortcuts
- System tray integration
- Audio file import/export
- Subtitle format support (SRT)
- Text formatting (plain and rich text)
- GPU acceleration support (CUDA, ROCm, Vulkan, OpenCL)
- Flatpak packaging with add-ons
- Cross-platform support (Linux, SailfishOS)

## [4.4.0] - 2024-01-25

### Added in 4.4.0

- Distil Faster-Whisper models (large-v3, small)
- License info to TTS models
- Option to enable/disable text in notifications
- Speed control for XTTS
- Lithuanian to English translator model
- Support for multiple audio streams in video file
- Coqui YourTTS models
- SFOS UI: 'Select audio stream' dialog
- SFOS UI: 'Download'/'Enable' in model browser
- SFOS UI: Translator progress indicator
- SFOS UI: Scroll to beginning/bottom buttons
- New RHVoice Uzbek models

### Fixed in 4.4.0

- Invalid path for translation files on SFOS
- Invalid JSON returned by Vosk lib
- Invalid CPU info parsing
- Bad decoding in faster-whisper
- Tiny package size
- UI glitches
- Audio normalization don't work
- Addon indicator always visible
- Crash in april-asr
- ARM32 build error
- Missing header file
- Invalid flatpak manifest
- Invalid timestamps in Vosk and April engines
- Translator model not visible on SFOS
- Translate-en option was enabled for English models
- Invalid timestamps when importing subtitles
- Empty tooltip
- Missing window icon on Wayland
- Crash when CUDA is missing
- Busy icon missing
- Typo errors

### Changed in 4.4.0

- Enable STT distil-large-v3 for English and Polish
- Update ctranslate2 to version 4.2.1
- Update ctranslate2 and faster-whisper
- Disable distil-large-v3 for faster-whisper
- Trim text before TTS when 'split text' is disabled
- Add XTTS model for Vietnamese
- Add new STT Whisper models
- Update Ukrainian and Russian translations
- Show NVIDIA kmod warning on main window
- Disable manual file path editing in Flatpak
- Update icon
- Always normalize TTS audio
- Use less aggressive denoiser in voice sample recorder
- Disable 'clean text' by default
- Update Swedish translation
- Update URLs for mimic3 models
- Update Italian translation
- Update Piper model for Farsi/Gyro
- Use older OpenBLAS on SFOS
- Update OpenBLAS
- Disable WhisperSpeech stdout
- Fallback to plain text in TTS
- Auto detect invalid text format
- Don't start TTS when task list is empty
- Show indicator when voice sample is needed
- Show indicator when Flatpak addon is not installed
- Don't show GPU settings on ARM
- Disable torch libs when no AVX
- CPU info refactor
- Respect timestamps in TTS
- Recognize SRT format in TTS
- Whisper.cpp update
- Coqui TTS update to version 0.22.0
- Reset 'additional caps' filter on model type change

## [4.3.0] - 2023-11-12

### Added in 4.3.0

- Faster-Whisper STT engine and models
- Global keyboard shortcuts
- Desktop notifications when listening
- Actions via DBus and command line options
- Start-listening-clipboard action
- TTS actions and global shortcuts
- Mimic3 TTS engine and models
- Support for keyboard layouts when sending key events
- Distil-Whisper medium model
- Speed control to main window UI
- More steps for speech speed
- Speech speed control for all VITS models
- Support for diacritics restoration (Arabic and Hebrew)
- Hebrew Coqui model
- Hebrew Whisper models
- More Piper models
- More MBROLA and eSpeak models
- Info about Faster-Whisper lib
- Delay for sending X11 key events
- Map non-ASCII characters to key events

### Fixed in 4.3.0

- Model 'count' wasn't updated
- Previous April result isn't cleared on stream EOF
- Mimic3 voice not found
- Invalid voice name
- Korean not split into sentences
- Sentences not merged correctly for April STT
- Coqui MMS models for Chinese were broken
- Typos
- OpenCL was disabled
- Correctly apply time stretch
- Video to transcribe file error
- Compilation error on SFOS
- Model download error

### Changed in 4.3.0

- Use tabs in settings
- Process already buffered samples on stop-listening action
- Improve desktop notifications
- Use tabs instead of buttons in UI
- Improve UI when screen is narrow
- Add English names to punctuation and diacritics models
- Improved April-asr integration
- Better time format
- Update Whisper.cpp
- Update Italian translation
- Update Swedish translation
- Add French and updated English models for April STT
- Add new Piper models
- Use AV logger
- Compressor improvements
- Use cache audio in compressed format
- Remember last filename in 'export to audio file'
- Add Opus codec to 'export to audio file'
- Remove cached files on exit
- Use lower log verbosity
- Don't show diacritizer warning when no Coqui TTS
- Keep 'decoding' state when stop listening
- Update Flatpak manifest
- Add extra pause for mimic3
- Pass CUDA device ID to diacritizer and punctuator
- Enable GPU for Hebrew diacritizer
- Enable GPU in punctuator
- Use qqc2 'org.kde.breeze' style on GNOME Flatpak
- Rise app window on desktop notification click
- Setting option to disable Python libs scan
- Install clblast lib
- Use correct rpath
- Scan for OpenCL only when no other devices
- Add HIP architectures to whisper.cpp build
- Execute action after feature availability update
- Dynamically disable not available models
- Use CUDA for Coqui TTS
- Use CUDA for faster-whisper
- Options to disable GPU devices scanning

## [4.2.1] - 2023-09-29

### Added in 4.2.1

- ROCm-OpenCL implementation

### Fixed in 4.2.1

- Minor bug fixes

## [4.2.0] - 2023-09-25

### Added in 4.2.0

- Opening files from command arguments and drag & drop
- Menu option to open and save text file
- CUBLAS and HIPBLAS Whisper versions
- Option to pause/resume reading
- Support for transcribing video files
- MP3 and Vorbis compression for saving to file
- Option to set audio input device
- TTS MMS model for Hungarian
- Dialog for save to file
- Meta-data tag writing to audio file
- More TTS MMS models
- New translator model
- RHVoice Uzbek model update
- Sharing support on SFOS
- Pause button for cover (SFOS)
- Command file interface
- Opening files via sharing (SFOS)

### Fixed in 4.2.0

- Invalid path to RHVoice data
- Can't set OpenCL device
- Crash on Whisper engine abort
- Minor HIPblas fixes

### Changed in 4.2.0

- Don't stop TTS when single sentence encoding fails
- Use URLs to Piper models pointing to specific commit
- Improved DBus activation
- Convert numbers to words for some TTS models
- Text tools refactor
- Update existing list items instead of replacing
- Update translations: Italian, Swedish, Dutch
- Ignore audio stream encoder errors
- Improved abort mechanism for whisper.cpp
- RHVoice engine update
- Trim sentence before TTS
- Upgrade Whisper.cpp
- Write track tag to audio file
- Don't read empty lines
- Support for writing meta-data tag to audio file
- Improved tool options description
- Update French translation
- Enable more Qt Quick styles
- Update README

## [4.1.0] - 2023-08-23

### Added in 4.1.0

- 10 more VCTK English voices (Piper)
- More Coqui models: Turkish, Japanese, Spanish
- Support for vocoder in Coqui models
- Option to change font size
- Chinese MMS models: Hakka, Min Nan
- Speed control for TTS
- New Piper voices
- eSpeak for Latin
- More MMS models
- MMS models for Korean and Amharic with romanizer
- Updated RHVoice voices for Slovak and Czech
- TTS models from Meta MMS project
- Option to enable and select GPU device
- Whisper engine with clblast
- Dutch translation
- Italian translation

### Fixed in 4.1.0

- Bengali eSpeak wasn't segmented into sentences
- Invalid ID for VITS Piper models
- Typos

### Changed in 4.1.0

- Add 'piper' to name of VCTK models
- Use default system font size
- Update Dutch translation
- Update Italian translation
- Disable Piper Alan voice
- More accurate names for VCTK models
- Use astrunc for sentence segmentation
- Added info about 3rd party libs and licenses
- Update README
- Update RHVoice
- Add TtsPlaySpeech2 DBus method with TTS options
- Log improvements
- Desktop UI improvements
- Don't use Fidel script on SFOS
- SFOS UI for speed control
- Use relative change of model length scale
- Use Rubberband lib only in x86_64
- Use pipe for executing Perl script
- Update language info
- Use 'ja' as language code for Japanese
- Enable eSpeak for more languages
- Enable Whisper for more languages
- Enable MMS models in Coqui
- Disable Whisper speed-up option
- Force speech timeout when Whisper decoding fails

## [4.0.0] - 2023-08-07

### Added in 4.0.0

- Offline neural machine translator (Bergamot)
- Firefox-translations models
- Support for speaker-id in Piper engine
- Many new Piper models for English
- RHVoice Slovak model
- Option to change Qt style
- More Piper models
- Swedish translation

### Fixed in 4.0.0

- On download error all models were deleted
- Incorrect include directory for Piper
- Deleting eSpeak model deletes all other models
- Incorrect path to BLAS lib
- Invalid XML tag
- Build error on ARM32
- Invalid speaker-id for RHVoice Leticia

### Changed in 4.0.0

- Update Piper to version 1.2.0
- Sort models by name instead of ID
- Desktop UI polish
- Translation files update
- Bergamot patch cleanup
- Fixes for ARM32 build
- UI and translation polish
- Better number of threads assignment
- Don't load too large Vosk models
- Desktop UI improvements
- Use custom build for ARM32 libstt
- Update Swedish and French translations
- Show changes when app starts after update
- Update README
- Add contribution section to README

## [3.1.6] - 2023-07-13

### Fixed in 3.1.6

- UI wasn't refreshed after default model change
- RHVoice Uzbek language config was missing

### Changed in 3.1.6

- Better number of threads assignment

## [3.1.5] - 2023-07-07

### Added in 3.1.5

- Coqui Jenny voice
- Screenshot from phone

### Fixed in 3.1.5

- Invalid XML tag
- Speaker-id for RHVoice Leticia was invalid
- Don't show changelog when other dialog is open
- Desktop UI wasn't updated on model change

### Changed in 3.1.5

- Don't load too large Vosk models
- Desktop UI improvements
- Use custom build for ARM32 libstt
- Downgrade libstt to version 1.1.0
- Add decoding time measurement log
- Use gender ID in TTS models
- Enable only finished translations
- Update Swedish and French translations
- Update translation files

## [3.1.4] - 2023-07-03

### Added in 3.1.4

- RHVoice model for Uzbek
- New RHVoice model for English
- New Piper model for Chinese
- Whisper large models and q5_0 quantization
- Whisper fine-tuned models for some languages
- Whisper models for all enabled languages
- Piper models for Swedish and Russian
- Piper and eSpeak models for Icelandic
- Punctuation model for Slovak and Danish

### Fixed in 3.1.4

- Build error on ARM32
- Piper and RHVoice on ARM64

### Changed in 3.1.4

- Better UI layout for small screens
- Show changes when app starts after update
- Update French translation
- Update README
- Use special libwhisper when CPU doesn't support AVX
- Use dlopen to access Whisper lib
- Build Whisper shared lib
- Enable Whisper small model on SFOS
- Update whisper-cpp and enable more threads
- Update Ukrainian RHVoice models
- Update RHVoice
- Update appdata
- Piper update
- Add instruction for non-flatpak build

## [3.1.3] - 2023-06-24

### Changed in 3.1.3

- Don't use language ID with postfix
- Use only first part of language code for Whisper engine
- Swedish translation update

## [3.1.2] - 2023-06-20

### Added in 3.1.2

- New Latvian DeepSpeech model
- Hello page
- Option to save speech to audio file
- Coqui TTS models
- Flatpak support (aarch64)

### Fixed in 3.1.2

- Missing settings properties
- Typos

### Changed in 3.1.2

- Use downloads directory as default for models on SFOS
- Update README
- Set main window size depending on screen size
- Disable JIT in PyTorch
- Update Flatpak files
- Update screenshots
- Use cache directory as default directory for models
- Install appdata file
- Improved program exiting
- Minor UI fixes
- Update Flatpak appstream data
- Update list of libraries
- Ignore Coqui TTS models when Python modules are disabled
- Try 3x to install module
- Update DBus API description
- App icon update
- UI update for speech write option
- Speed up transcription from file
- Higher default score for Coqui TTS models
- Better handling lack of default model
- Increase audio chunk for file transcription
- New desktop UI
- Icon update

## [3.1.1] - 2023-06-17

### Added in 3.1.1

- More Vosk models
- mbrola models
- eSpeak models and TTS engine
- Punctuation models
- Advanced punctuation restoration option
- RHVoice TTS engine
- Piper TTS engine
- Option to set default TTS model
- TTS feature with text-to-speech support
- Option to split text into sentences for TTS
- STT/TTS language list to DBus API

### Fixed in 3.1.1

- Don't write WAV when eSpeak returns no data
- Invalid option name
- Ignore WAV data when TTS is shutting down
- Typos

### Changed in 3.1.1

- Disable some Vosk models on SFOS
- Add more logging
- Use English NB data as default
- Don't show 'tube' icon for model with 0 score
- Use lower score for eSpeak languages
- Update README
- Update translation files
- Minor cleanup
- Improved names in UI
- Show info to download punctuation model
- Remove cached WAV files on service start
- Piper models configuration
- UI fixes
- Refactor: split CMake script into multiple files
- Less debug logs
- Initialize Python module async
- Restore punctuation
- Add verbose command option
- Better names
- Command option for standalone launch mode

## [3.0.0] - 2023-05-22

### Added in 3.0.0

- RHVoice TTS engine
- eSpeak TTS engine
- Piper TTS engine
- Coqui TTS models
- mbrola models
- Punctuation restoration
- TTS (Text-to-Speech) feature
- Option to save speech to audio file
- Hello/welcome page
- Flatpak support

### Changed in 3.0.0

- App description changed
- Disable some Vosk models on SFOS
- Major UI improvements
- New desktop UI
- Update icon
- Update list of libraries

## [2.0.1] - 2023-04-15

### Added in 2.0.1

- RNNoise for noise reduction
- Swedish translation
- Dutch translation

### Fixed in 2.0.1

- Use Swedish language name
- Audio debugging improvements

### Changed in 2.0.1

- Improved denoising
- Update Whisper.cpp
- Refactor: rename vad_wrapper to vad
- Use aliases for multilang models
- Add clang-format config
- Add stt_ prefix to engine type
- Remove noise with RNNoise
- Use up to 2 threads for Whisper
- Update Dutch translation

## [2.0.0] - 2023-04-07

### Added in 2.0.0

- Vosk STT engine and models
- Whisper STT engine (whisper.cpp)
- Text append style option
- Option to translate to English
- Option to mark model as default for language
- 'Score' property for models
- Many new Vosk models
- Swahili Coqui model
- Persian DeepSpeech model

### Fixed in 2.0.0

- Minor app bugs
- Missing build dependency
- Lack of language ID

### Changed in 2.0.0

- Update translations
- Disable Whisper small and medium models on SFOS
- Reset engine always in processing thread
- Minor UI fixes
- Build defaults for SFOS
- Models names and score changes
- Improved STT engine handling
- Do multithreaded XZ decode
- Update languages/model info
- Keep STT model loaded if model file doesn't change
- Use additional speech status when model loads
- Disable large Vosk models for SFOS
- Disable Whisper for ARM32 without NEON-FP
- Improved logging
- Detect CPU flags in runtime for Whisper
- Use shared OpenBLAS build for ARM
- Czech Commodoro model update
- French Common Voice model update
- Don't log decoded text in non-debug build
- Don't use newline when appending decoded text
- Use 'score' property for models
- File source with VAD improvements
- Limit number of Whisper threads to 2
- Better logging
- Use Latin script for Serbian Whisper models
- Update README
- Link LAPACK statically
- VAD improvements
- Better handling of audio source EOF
- Mark all Whisper small/medium models as experimental

## [1.8.0] - 2022-04-02

### Added in 1.8.0

- Finnish model and scorer

### Fixed in 1.8.0

- Incorrect app icon

### Changed in 1.8.0

- STT lib downgrade to v1.1.0
- Disable sandboxing
- Update translation files
- Handle STT error
- New config scheme
- New model for Polish language
- Remove experimental model de_medn
- Better config version discovery
- STT lib update to v1.3.0
- Make Finnish model not experimental
- Search by language English name on languages list

## [1.7.0] - 2022-03-04

### Added in 1.7.0

- New experimental models: de-med, de-medn
- New models: Estonian, English, Finnish, Mongolian
- Option to cancel ongoing download
- Qt logger

### Fixed in 1.7.0

- Bugs discovered with clang-tidy

### Changed in 1.7.0

- Remove unused include file
- Don't re-download model/scorer files if already downloaded
- Apply clang-format
- Enable sanitizers in debug build
- Remove show experimental option
- Models grouped by languages
- Use more QStringLiteral

## [1.6.1] - 2021-12-10

### Added in 1.6.1

- New experimental model for German language

### Changed in 1.6.1

- Reset listening mode to default

## [1.6.0] - 2021-12-09

### Added in 1.6.0

- One-sentence listening mode
- Experimental models feature
- Model for Ukrainian language
- Model for Russian language
- Model for Romanian language (later removed)
- DBus API
- Systemd service

### Fixed in 1.6.0

- Create data directory when it does not exist
- Proper path for systemd user unit on ARM64
- Disable transcribe option when no language is configured
- lib64 directory for aarch64
- Typo

### Changed in 1.6.0

- Update translation files
- Coqui libstt update to v1.1.0
- Improved language download
- Do not ignore SSL errors
- Quick checksum to speed up service start
- Update README
- Models property on DBus API
- Store default models configuration in separated JSON file
- Use CRC32 instead of MD5 as checksum
- Don't show empty pull menu
- Better error handling

## [1.5.1] - 2021-11-17

### Changed in 1.5.1

- Minor version update

## [1.5.0] - 2021-11-16

### Added in 1.5.0

- Romanian language model (later removed)
- Ukrainian language model
- Russian language model
- DBus API
- Systemd service
- Models configuration in JSON

### Changed in 1.5.0

- Better error handling
- Update README

## [1.4.0] - 2021-11-14

### Added in 1.4.0

- Romanian language model
- Use Coqui's libstt instead of libdeepspeech

### Fixed in 1.4.0

- Missing header file

### Changed in 1.4.0

- Remove old model files
- Updated ARM build of libstt
- Remove models files

## [1.3.0] - 2021-10-01

### Added in 1.3.0

- French language model from Common Voice
- Italian language model from MozillaItalia
- Support for model files compressed with tar.xz
- Support for model files compressed with gzip
- Support for model files divided into parts

### Changed in 1.3.0

- Using github.com/MozillaItalia/DeepSpeech-Italian-Model as source for Italian model
- Using github.com/rhasspy as download source for models
- Native name of Czech language

## [1.2.0] - 2021-09-18

### Added in 1.2.0

- Support for compressed model files
- Support for split model files
- Italian language support
- French language support

### Changed in 1.2.0

- Improved model download system

## [1.0.0] - 2021-04-22

### Added in 1.0.0

- Initial release
- Speech-to-Text using Mozilla DeepSpeech/Coqui STT
- Support for multiple languages
- Model download and management
- SailfishOS UI
- Basic transcription features
