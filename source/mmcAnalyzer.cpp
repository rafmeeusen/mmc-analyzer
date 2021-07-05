#include "mmcAnalyzer.h"
#include "mmcAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

mmcAnalyzer::mmcAnalyzer()
:	Analyzer2(),  
	mSettings( new mmcAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}


mmcAnalyzer::~mmcAnalyzer()
{
	KillThread();
}

void mmcAnalyzer::SetupResults()
{
	mResults.reset( new mmcAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mCommandChannel );
}



U32 mmcAnalyzer::ReadAndMarkCmdBits(U8 nrOfBits)
{
    if ( nrOfBits > 32 ) {
        mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mCommandChannel );
        return 0;
    }

    U32 data = 0;
    U32 mask = 1 << (nrOfBits-1);
    Frame frame;
    frame.mStartingSampleInclusive = mClock->GetSampleNumber();
    for (int i=0; i<nrOfBits; i++) {
        if ( GetCommandBit() ) {
            data |= mask;
        } 
        mask = mask >> 1;
        mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mCommandChannel );
    } 
    frame.mData1 = data ;
    frame.mFlags = 0;
    frame.mEndingSampleInclusive = mCommand->GetSampleNumber();
    mResults->AddFrame( frame );
    ReportProgress( frame.mEndingSampleInclusive );
    return data;
}

// idea: get value of command on current rising edge of clock
//       AND: forward command stream to this sample
//       AND: forward clock already to next rising edge
bool mmcAnalyzer::GetCommandBit() {
    U64 risingedge_sample = mClock->GetSampleNumber();
    mCommand->AdvanceToAbsPosition( risingedge_sample ) ;
    U64 cmdvalue = mCommand->GetBitState();
    mClock->AdvanceToNextEdge();
    mClock->AdvanceToNextEdge();

    return ( cmdvalue == BIT_HIGH );
}


enum FrameType mmcAnalyzer::SMgetExpected() {
    return mmcAnalyzer::nextExpected;
}

void mmcAnalyzer::SMinit() {
    mmcAnalyzer::nextExpected = MMC_CMD;
}

void mmcAnalyzer::SMputActual( enum FrameType frame ) {
    // most simple: cmd <--> rsp of 48 bits
    switch (frame) {
        case MMC_CMD:
            mmcAnalyzer::nextExpected = MMC_RSP_48;
            break;
        case MMC_CMD2:
            mmcAnalyzer::nextExpected = MMC_RSP_136;
            break;
        case MMC_RSP_48:
            mmcAnalyzer::nextExpected = MMC_CMD;
            break;
        case MMC_RSP_136:
            mmcAnalyzer::nextExpected = MMC_CMD;
            break;
    }
}



void mmcAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();

	mClock = GetAnalyzerChannelData( mSettings->mClockChannel );
	mCommand = GetAnalyzerChannelData( mSettings->mCommandChannel );

    // init: advance clk to first low sample, then to first rising edge
    if ( mClock->GetBitState() == BIT_HIGH ) {
        mClock->AdvanceToNextEdge();
    } else {
        // nothing; clock is low
    }
    mClock->AdvanceToNextEdge();
    // CLK ON RISING EDGE NOW

    SMinit();

// { Dot, ErrorDot, Square, ErrorSquare, UpArrow, DownArrow, X, ErrorX, Start, Stop, One, Zero }
    for( ; ; )
    { 

        // find start bit:
        bool cmdbit;
        do {
            cmdbit = GetCommandBit();    
        } while (cmdbit);

        mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Start, mSettings->mCommandChannel );

        enum FrameType expectedFrame = SMgetExpected();
        bool transmission_bit;
        bool endbit;
        U32 cmdidx;
        switch ( expectedFrame ) {
            case MMC_CMD:
                transmission_bit = GetCommandBit();
                if ( ! transmission_bit ) {
                    // expecting command, i.e. host talking
                    // ERROR! 
                }
                cmdidx = ReadAndMarkCmdBits(6);
                ReadAndMarkCmdBits(32);
                ReadAndMarkCmdBits(7);
                endbit = GetCommandBit();
                if ( endbit ) {
                    mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mCommandChannel );
                } else {
                    mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mCommandChannel );
                }
                mResults->CommitResults();
                if ( cmdidx == 2 ) { 
                    SMputActual( MMC_CMD2 );
                } else { 
                    SMputActual( MMC_CMD );
                }

                break;
            case MMC_RSP_48:
                transmission_bit = GetCommandBit();
                if ( transmission_bit ) {
                    // ERROR! 
                }
                cmdidx = ReadAndMarkCmdBits(6); // depends on response type
                ReadAndMarkCmdBits(32); // card status
                ReadAndMarkCmdBits(7);  // crc
                endbit = GetCommandBit();
// { Dot, ErrorDot, Square, ErrorSquare, UpArrow, DownArrow, X, ErrorX, Start, Stop, One, Zero }
                if ( endbit ) {
                    mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Square, mSettings->mCommandChannel );
                } else {
                    mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mCommandChannel );
                }
                mResults->CommitResults();
                SMputActual( MMC_RSP_48 );
                break; 
            case MMC_RSP_136:
                transmission_bit = GetCommandBit();
                if ( transmission_bit ) {
                    // ERROR! 
                }
                ReadAndMarkCmdBits(6); // for R2: 6 high level check bits.
               
                ReadAndMarkCmdBits(32); // CID or CSD incl. internal CRC7
                ReadAndMarkCmdBits(32); // cont
                ReadAndMarkCmdBits(32); // cont
                ReadAndMarkCmdBits(24); // cont 
                ReadAndMarkCmdBits(7); // CRC inside CID/CSD 
                endbit = GetCommandBit();
                if ( endbit ) {
                    mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::X, mSettings->mCommandChannel );
                } else {
                    mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mCommandChannel );
                }
                SMputActual( MMC_RSP_136 );
                break;
            default:
                // not expecting this...
                break;
        }


    }



}

bool mmcAnalyzer::NeedsRerun()
{
	return false;
}

U32 mmcAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 mmcAnalyzer::GetMinimumSampleRateHz()
{
	return 9600 * 4;
}

const char* mmcAnalyzer::GetAnalyzerName() const
{
	return "mmc";
}

const char* GetAnalyzerName()
{
	return "mmc";
}

Analyzer* CreateAnalyzer()
{
	return new mmcAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
