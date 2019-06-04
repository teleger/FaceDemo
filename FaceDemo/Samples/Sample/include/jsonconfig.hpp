#ifndef __JSONCONFIG_HPP
#define __JSONCONFIG_HPP

#include <json/value.h>
#include <json/reader.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include<stdio.h>

class Jsonconfig{
private:
    const std::string TAG = "JsonConfigure";
public:
    Jsonconfig(){};
    ~Jsonconfig(){
        if(ifs.is_open()){
            ifs.close();
        }
    };

    void write();
    Json::Reader reader;
    Json::Value  root;

   bool open(const std::string& file);

    void read();

    void parse_all(int m_GnetType);

public:
    std::ifstream ifs;
public:
    std::string    similar;
    std::string   m_coefName;
    std::string   userConfig;
    std::string   m_faceFcname;
    std::string   m_faceLabelname;
    std::string   m_picFcname;
    std::string   m_picLabelname;
    std::string   m_videoFcname;
    std::string   m_videoLabelname;
    std::string   m_camFcname;
    std::string   m_camLabelname;
    std::string   m_camFc_pets;
    std::string   m_camLabel_pets;
    std::string   m_Fc_office;
    std::string   m_Label_office;
    std::string   m_Fc_kitchen;
    std::string   m_Label_kitchen;
    std::string   m_Fc_livingroom;
    std::string   m_Label_livingroom;
};

#endif