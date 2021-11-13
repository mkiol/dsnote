DeepSpeech models configuration is stored in JSON format.

Following schema applies:

(M) - mandatory
(O) - optional

{ 
    "version": <config_version>,
    "models: [
        {
            "name": "<native language name (M)>",
            "model_id": "unique model id (M)>",
            "lang_id": "<ISO 639-1 language code (M)>",
            "urls": "<array of download URL(s) of model file (*.tflite), might be compressd file(s) (M)>",
            "checksum": "<CRC-32 hash of (not compressed) model file (M)>",
            "file_name": "<file name of deep-speech model (O)>",
            "comp": <type of compression for model file provided in 'url', following are supported: 'xz', 'gz' (O)>
            "size": "<size in bytes of file provided in 'url' (O)>",
            "scorer_urls": "<array download URL(s) of scorer file, might be compressd file(s) (O)>",
            "scorer_checksum": "<CRC-32 hash of (not compressed) scorer file (M if scorer is provided)>",
            "scorer_file_name": "<file name of deep-speech scorer (O)>",
            "scorer_comp": <type of compression for scorer file provided in 'scorer_url', following are supported: 'xz', 'gz', 'tarxz' (O)>
            "scorer_size": "<size in bytes of file provided in 'scorer_url' (O)>"
        } 
    ]
}
