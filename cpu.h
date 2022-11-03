#pragma once

// qt
#include <QtCore/QString>


class CpuInfo
{
public:
    static QString GetBrandName();
    static bool GetCores(unsigned int &sockets, unsigned int &physical, unsigned int &logicals);
};
