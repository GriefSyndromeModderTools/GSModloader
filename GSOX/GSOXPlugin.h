#pragma once

class GSOXPlugin : public Plugin
{
public:
	GSOXPlugin() : Plugin(_T("GSOX"))
	{
	}

	virtual void InjectData();

public:
	HINSTANCE hInstanceDll;

	static GSOXPlugin& GetInstance();
};
