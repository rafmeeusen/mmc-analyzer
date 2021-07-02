#ifndef MMC_ANALYZER_SETTINGS
#define MMC_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class mmcAnalyzerSettings : public AnalyzerSettings
{
public:
	mmcAnalyzerSettings();
	virtual ~mmcAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();
	
	Channel mClockChannel;
	Channel mCommandChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mClockChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mCommandChannelInterface;
};

#endif //MMC_ANALYZER_SETTINGS
