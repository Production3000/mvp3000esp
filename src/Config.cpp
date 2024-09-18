/* 
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
*/

#include "Config.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void Config::setup() {
    // On ESP32 this can be called with setup(true) to format the FS automatically
    // Contrary to documentation, it (I think) defaults to not do this automatically
    if (SPIFFS.begin()) {
        fileSystemOK = true;
        return;
    }

    // Code should only be executed for factory-new ESP, otherwise device likely broken beyond repair
    mvp.logger.write(CfgLogger::Level::WARNING, "Failed to mount file system. Formatting ...");
    if (!SPIFFS.format()) {
        mvp.logger.write(CfgLogger::Level::ERROR, "Formatting file system failed.");
        SPIFFS.end();
        return;
    }

    // Check FS again
    if (SPIFFS.begin()) {
        fileSystemOK = true;
        return;
    }

    SPIFFS.end();
    mvp.logger.write(CfgLogger::Level::ERROR, "Permanently failed to mount file system.");
}

void Config::loop() {
    // Check if delayed factory reset was started
    if (delayedFactoryReset_ms > 0) {
        if (millis() > delayedFactoryReset_ms) {
            delayedFactoryReset_ms = 0;
            factoryResetDevice(delayedFactoryResetKeepWifi);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Config::isReadyFS() {
    if (fileSystemOK)
        return true;

    mvp.logger.write(CfgLogger::Level::ERROR, "Permanent file system error.");
    return false;
}

void Config::readCfg(JsonInterface &cfg) {
    if (!readFileToJson(cfg.cfgName.c_str()))
        return;
    // Import settings from JSON
    if (!cfg.importFromJson(jsonDoc)) {
        // JSON vs. content mismatch, remove file
        removeFile(cfg.cfgName.c_str());
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Mismatch between loaded JSON and expected content, deleting: %s", cfg.cfgName.c_str());
    } else {
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Config loaded: %s", cfg.cfgName.c_str());
    }
    // Cleanup for next
    jsonDoc.clear(); 
}

void Config::writeCfg(JsonInterface &cfg) {
    if (!jsonDoc.isNull()) {
        mvp.logger.write(CfgLogger::Level::WARNING, "JSON doc was not empty.");
        jsonDoc.clear();
    }
    // Export settings to JSON
    cfg.exportToJson(jsonDoc);
    // Write to file
    writeJsonToFile(cfg.cfgName.c_str());
}

bool Config::readFileToJson(const char* fileName) {
    // Check if all other operations are done (this is mainly useful while coding)
    if (!jsonDoc.isNull()) {
        mvp.logger.write(CfgLogger::Level::WARNING, "JSON doc was not empty.");
        jsonDoc.clear();
    }

    // Check if the file exists, then check content 
    return readFile(fileName, [&](File& jsonFile) -> bool {
        // Deserialize JSON
        DeserializationError error = deserializeJson(jsonDoc, jsonFile);

        if (error != DeserializationError::Ok)
            switch (error.code()) {
                case DeserializationError::EmptyInput:
                case DeserializationError::IncompleteInput:
                case DeserializationError::InvalidInput:
                    mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Config JSON is invalid: %s", fileName);
                    return false;
                case DeserializationError::NoMemory:
                    mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Config JSON Not enough memory: %s", fileName);
                    return false;
                case DeserializationError::TooDeep:
                    mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Config JSON too deep: %s", fileName);
                    return false;
                default:
                    mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Config JSON deserialization failed: %s", fileName);
                    return false;
            }

        return true;
    });
}

void Config::writeJsonToFile(const char* fileName) {
    // Empty cfg, remove file if any
    if (jsonDoc.isNull()) {
        removeFile(fileName);
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Empty cfg, cancel saving: %s", fileName);
        return;
    }

    writeFile(fileName, [&](File& jsonFile) -> bool {
        return serializeJson(jsonDoc, jsonFile) != 0;
    });
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Config written: %s", fileName);

    // Clean up for next
    jsonDoc.clear();
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void Config::asyncFactoryResetDevice(boolean keepWifi) {
    // This is a workaround to not trigger the ESP32 watchdog on the web server, it still floods the serial
    // Also it allows to show a message on the web server while the device is resetting
    delayedFactoryReset_ms = millis() + 25;
    delayedFactoryResetKeepWifi = keepWifi;
}

void Config::factoryResetDevice(boolean keepWifi) {
    if (!isReadyFS())
        return;

    mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Starting factory reset ...");

    // Clear any saved data, factory config will be restored to defaults on reboot
    // Triggers watchdog _a_lot_, but does not cause reboot
    SPIFFS.format();

    // Re-save wifi client settings if requested
    if (keepWifi) {
        mvp.net.cleanCfgKeepClientInfo();
    }
    
    mvp.delayedRestart(25);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void Config::removeFile(const char* fileName) {
    if (!fileSystemOK)
        return;

    SPIFFS.remove(fileNameCompletor(fileName));
}

String Config::fileNameCompletor(const char* fileName) {
    // Remove leading '/', there should be none
    if (fileName[0] == '/')
        fileName = fileName + 1;
    // Add leading '/' and ending .json
    return "/" + String(fileName) + ".json";
}

bool Config::readFile(const char* fileName, std::function<bool(File& file)> readerFunc) {
    if (!fileSystemOK)
        return false;

    String pathFileName = fileNameCompletor(fileName);

    File file = SPIFFS.open(pathFileName, "r");
    // It's no longer enough to check if open returned true. Also need to check that it is not a folder. The documentations needs to be updated. ;) 
    if (!file || file.isDirectory()) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "File not found for reading: %s", pathFileName.c_str());
        return false;
    }

    // Empty file, remove. This should never happen I think.
    if (file.size() == 0) {
        SPIFFS.remove(pathFileName);
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "File empty, removed: %s", pathFileName.c_str());
        return false;
    }
    
    // Load good
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Content read from file: %s", pathFileName.c_str());

    // Execute the read function, specific to content type: json or custom
    boolean result = readerFunc(file);

    file.close();
    return result;
}

bool Config::writeFile(const char* fileName, std::function<bool(File& file)> writerFunc) {   
    if (!fileSystemOK)
        return false;

    String pathFileName = fileNameCompletor(fileName);

    // Open file, automatically created if not existing
    File file = SPIFFS.open(pathFileName, "w");
    // It's no longer enough to check if open returned true. Also need to check that it is not a folder. The documentations needs to be updated. ;) 
    if (!file || file.isDirectory()) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Failed to open file for writing: %s", pathFileName.c_str());
        return false;
    }
    
    // Execute the write function, specific to content type
    if (!writerFunc(file)) {
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Failed to write to file: %s", pathFileName.c_str());
        file.close();
        return false;
    }

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Content written to file: %s", pathFileName.c_str());
    file.close();
    return true;
}
