Language models configuration is stored in JSON format.

Following schema applies:

(M) - mandatory
(O) - optional

{ 
    "version": <config_version>,
    "langs": [
        {
            "name": "<native language name (M)>",
            "id": "<ISO 639-1 language code (M)>"
        }
    ],
    "models: [
        {
            "name": "<model name (M)>",
            "model_id": "<unique model id (M)>",
            "model_alias_of": "<already defined model id (O)>"
            "engine": "<engine type: 'stt_ds', 'stt_vosk' or 'stt_whisper' (M when 'model_alias_of' is not present)>",
            "lang_id": "<ISO 639-1 language code (M)>",
            "score": <score 0-5 of usability of this model (O)>,
            "urls": "<array of download URL(s) of model file (*.tflite), might be compressd file(s) (M when 'model_alias_of' is not present)>",
            "checksum": "<CRC-32 hash of (not compressed) model file (M when 'model_alias_of' is not present)>",
            "checksum_quick": "<CRC-32 hash of (not compressed) model file (only first and last 65535 bytes) (O)>",
            "file_name": "<file name of deep-speech model (O)>",
            "comp": <type of compression for model file provided in 'url', following are supported: 'xz', 'gz' (O)>
            "size": "<size in bytes of file provided in 'url' (O)>",
            "scorer_urls": "<array download URL(s) of scorer file, might be compressd file(s) (O)>",
            "scorer_checksum": "<CRC-32 hash of (not compressed) scorer file (M when 'model_alias_of' is not present and scorer is provided)>",
            "scorer_checksum_quick": "<CRC-32 hash of (not compressed) scorer file (only first and last 65535 bytes) (M when 'model_alias_of' is not present and scorer is provided)>",
            "scorer_file_name": "<file name of deep-speech scorer (O)>",
            "scorer_comp": <type of compression for scorer file provided in 'scorer_url', following are supported: 'xz', 'gz', 'tarxz' (O)>
            "scorer_size": "<size in bytes of file provided in 'scorer_url' (O)>"
        } 
    ]
}
