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
    cfg.importFromJson(jsonDoc);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Config loaded: %s", cfg.cfgName.c_str());
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

bool Config::readFileToJson(const char *fileName) {
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

void Config::writeJsonToFile(const char *fileName) {
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



void Config::factoryResetDevice() {
    if (!isReadyFS())
        return;

    // Clear all stored values, main config will be set to defaults on reboot
    SPIFFS.format();
    
    // Wait for filesystem to actually write to flash
    SPIFFS.end();
    mvp.delayedRestart(25);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void Config::removeFile(const char *fileName) {
    if (!fileSystemOK)
        return;

    // Remove leading '/', there should be none
    if (fileName[0] == '/')
        fileName = fileName + 1;
    // Add leading '/', copy filename one character shifted, add .json
    char pathFileName[strlen(fileName) + 1 + 5];
    pathFileName[0] = '/';
    strcpy(pathFileName + 1, fileName);
    strcpy(pathFileName + 1 + strlen(fileName), ".json");

    SPIFFS.remove(pathFileName);
}


bool Config::readFile(const char *fileName, std::function<bool(File& file)> readerFunc) {
    if (!fileSystemOK)
        return false;

    // Remove leading '/', there should be none
    if (fileName[0] == '/')
        fileName = fileName + 1;
    // Add leading '/', copy filename one character shifted, add .json
    char pathFileName[strlen(fileName) + 1 + 5];
    pathFileName[0] = '/';
    strcpy(pathFileName + 1, fileName);
    strcpy(pathFileName + 1 + strlen(fileName), ".json");

    File file = SPIFFS.open(pathFileName, "r");
    // It's no longer enough to check if open returned true. Also need to check that it is not a folder. The documentations needs to be updated. ;) 
    if (!file || file.isDirectory()) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "File not found for reading: %s", pathFileName);
        return false;
    }

    // Empty file, remove. This should never happen I think.
    if (file.size() == 0) {
        SPIFFS.remove(pathFileName);
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "File empty, removed: %s", pathFileName);
        return false;
    }
    
    // Load good
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Content read from file: %s", pathFileName);

    // Execute the read function, specific to content type: json or custom
    boolean result = readerFunc(file);

    file.close();
    return result;
}

bool Config::writeFile(const char *fileName, std::function<bool(File& file)> writerFunc) {   
    if (!fileSystemOK)
        return false;

    // Remove leading '/', there should be none
    if (fileName[0] == '/')
        fileName = fileName + 1;
    // Add leading '/', copy filename one character shifted, add .json
    char pathFileName[strlen(fileName) + 1 + 5];
    pathFileName[0] = '/';
    strcpy(pathFileName + 1, fileName);
    strcpy(pathFileName + 1 + strlen(fileName), ".json");

    // Open file, automatically created if not existing
    File file = SPIFFS.open(pathFileName, "w");
    // It's no longer enough to check if open returned true. Also need to check that it is not a folder. The documentations needs to be updated. ;) 
    if (!file || file.isDirectory()) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Failed to open file for writing: %s", pathFileName);
        return false;
    }
    
    // Execute the write function, specific to content type
    if (!writerFunc(file)) {
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Failed to write to file: %s", pathFileName);
        file.close();
        return false;
    }

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Content written to file: %s", pathFileName);
    file.close();
    return true;
}



/* 
void testJsonConversion() {
    mvp.config.cfgWritePrepare();

    // uint8_t xuint = 123;
    // float_t xfloat = 123.567;
    // uint8_t yuint;
    // float_t yfloat;
    // mvp.config.cfgWriteAddValue("xuint", xuint);
    // mvp.config.cfgWriteAddValue("xfloat", xfloat);
    // mvp.config.readFileToJson("dummyprint");
    // mvp.config.cfgReadGetValue("xuint", yuint);
    // mvp.config.cfgReadGetValue("xfloat", yfloat);
    // Serial.print(yuint);
    // Serial.print(" | ");
    // Serial.print(yfloat);
    // Serial.print(" | ");


    // int16_t xarray[3] = {123,-456,789};
    // uint16_t xuarray[3] = {123,456,789};
    // float_t xfarray[3] = {123.456,-456.789,789.123};
    // int16_t yarray[3];
    // uint16_t yuarray[3];
    // float_t yfarray[3];
    // mvp.config.cfgWriteAddValue("xarray", xarray, 3);
    // mvp.config.cfgWriteAddValue("xuarray", xuarray, 3);
    // mvp.config.cfgWriteAddValue("xfarray", xfarray, 3);
    // mvp.config.readFileToJson("dummyprint");
    // mvp.config.cfgReadGetValue("xarray", yarray, 3);
    // mvp.config.cfgReadGetValue("xuarray", yuarray, 3);
    // mvp.config.cfgReadGetValue("xfarray", yfarray, 3);
    // Serial.print(yarray[0]);
    // Serial.print(" ");
    // Serial.print(yarray[1]);
    // Serial.print(" ");
    // Serial.print(yarray[2]);
    // Serial.print(" | ");
    // Serial.print(yuarray[0]);
    // Serial.print(" ");
    // Serial.print(yuarray[1]);
    // Serial.print(" ");
    // Serial.print(yuarray[2]);
    // Serial.print(" | ");
    // Serial.print(yfarray[0]);
    // Serial.print(" ");
    // Serial.print(yfarray[1]);
    // Serial.print(" ");
    // Serial.print(yfarray[2]);
    // Serial.print(" | ");


    const char *xcchar = "qweqwe";
    char xbchar[12] = "yxcyxc";
    char *xpchar = "asdasd";
    // String xstring = "qweqwe";
    const char* ycchar;
    char ybchar[12];
    char* ypchar;
    // String ystring;
    mvp.config.cfgWriteAddValue("xpchar", xpchar);
    mvp.config.cfgWriteAddValue("xcchar", xcchar);
    mvp.config.cfgWriteAddValue("xbchar", xbchar);
    // mvp.config.cfgWriteAddValue("xstring", xstring);
    mvp.config.readFileToJson("dummyprint");
    // mvp.config.cfgReadGetValue("xcchar", ycchar);
    // mvp.config.cfgReadGetValue("xbchar", ybchar);
    // mvp.config.cfgReadGetValue("xpchar", ypchar);
    // mvp.config.cfgReadGetValue("xstring", ystring);
    Serial.print(ypchar);
    Serial.print(" | ");
    Serial.print(ycchar);
    Serial.print(" | ");
    Serial.print(ybchar);
    Serial.print(" | ");


    Serial.println("");
    mvp.config.writeJsonToFile();
}
*/
