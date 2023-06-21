#include "stdafx.h"

int  CGuiSettings::m_RefCount = 0;
bool CGuiSettings::m_bCPURunning;

CGuiSettings::CGuiSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running,nullptr,RefreshSettings);
        RefreshSettings(nullptr);
    }
}

CGuiSettings::~CGuiSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Running,nullptr,RefreshSettings);
    }
}

void CGuiSettings::RefreshSettings(void *)
{
    m_bCPURunning = g_Settings->LoadBool(GameRunning_CPU_Running);
}
