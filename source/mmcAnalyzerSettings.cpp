#include "mmcAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


mmcAnalyzerSettings::mmcAnalyzerSettings()
:	mClockChannel( UNDEFINED_CHANNEL ),
    mCommandChannel ( UNDEFINED_CHANNEL )
	
{
	mClockChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mClockChannelInterface->SetTitleAndTooltip( "CLK", "clock" );
	mClockChannelInterface->SetChannel( mClockChannel );

	mCommandChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mCommandChannelInterface->SetTitleAndTooltip( "CMD", "command" );
	mCommandChannelInterface->SetChannel( mCommandChannel );

	AddInterface( mClockChannelInterface.get() );
	AddInterface( mCommandChannelInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mClockChannel, "Clock", false );
	AddChannel( mCommandChannel, "Command", false );
}

mmcAnalyzerSettings::~mmcAnalyzerSettings()
{
}

bool mmcAnalyzerSettings::SetSettingsFromInterfaces()
{
	mClockChannel = mClockChannelInterface->GetChannel();
	mCommandChannel = mCommandChannelInterface->GetChannel();

	ClearChannels();
	AddChannel( mClockChannel, "clk", true );
	AddChannel( mCommandChannel, "cmd", true );

	return true;
}

void mmcAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mClockChannelInterface->SetChannel( mClockChannel );
	mCommandChannelInterface->SetChannel( mCommandChannel );
}

void mmcAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mClockChannel;
	text_archive >> mCommandChannel;

	ClearChannels();
	AddChannel( mClockChannel, "mmc", true );
	AddChannel( mCommandChannel, "mmc", true );

	UpdateInterfacesFromSettings();
}

const char* mmcAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mClockChannel;
	text_archive << mCommandChannel;

	return SetReturnString( text_archive.GetString() );
}
