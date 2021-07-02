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



void mmcAnalyzer::ReadAndMarkCmdBits(U8 nrOfBits)
{
    if ( nrOfBits > 32 ) {
        mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mCommandChannel );
        return;
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


// { Dot, ErrorDot, Square, ErrorSquare, UpArrow, DownArrow, X, ErrorX, Start, Stop, One, Zero }
	for( ; ; )
    //for (int i=0; i<7;i++) 
    { 

        // find start of first cmd:
        bool cmdbit;
        do {
            cmdbit = GetCommandBit();    
        } while (cmdbit);

        mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Start, mSettings->mCommandChannel );

        // first bit is 0, now get 47 more bits
        bool transmission_bit = GetCommandBit();
        if ( transmission_bit ) {
            // host talking
        } else {
            // card talking
        }
            // get 6 cmd index bits
            ReadAndMarkCmdBits(6);
            ReadAndMarkCmdBits(32);
            ReadAndMarkCmdBits(7);
            bool endbit = GetCommandBit();
            if ( endbit ) {
                mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mCommandChannel );
            } else {
                mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mCommandChannel );
            }
            mResults->CommitResults();


    }


	//for( ; ; )
	{
		U8 data = 0;
		U8 mask = 1 << 7;
//		if( mCommand->GetBitState() == BIT_HIGH )
		U64 starting_sample = mClock->GetSampleNumber();
//		mClock->Advance( samples_to_first_center_of_first_data_bit );

		for( U32 i=0; i<8; i++ )
		{
 //          mClock->AdvanceToNextEdge();
 //          mClock->AdvanceToNextEdge(); 
			//let's put a dot exactly where we sample this bit:
//			if( mClock->GetBitState() == BIT_HIGH )
//				data |= mask;
//			mClock->Advance( samples_per_bit );
//			mask = mask >> 1;
		}

        data = 0x22;
		//we have a byte to save. 
		Frame frame;
		frame.mData1 = data;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = starting_sample;
		frame.mEndingSampleInclusive = mClock->GetSampleNumber();
//		mResults->AddFrame( frame );
//		mResults->CommitResults();
		//ReportProgress( frame.mEndingSampleInclusive );
	}


/////////////////////////////////////////////////
////// check if these two stops are still printed:
    for (int j=0; j<2; j++) mClock->AdvanceToNextEdge();
    mCommand->AdvanceToAbsPosition( mClock->GetSampleNumber() ) ;
	mResults->AddMarker( mClock->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mClockChannel );
	mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mCommandChannel );
    mResults->CommitResults();

    for (int j=0; j<2; j++) mClock->AdvanceToNextEdge();
    mCommand->AdvanceToAbsPosition( mClock->GetSampleNumber() ) ;
	mResults->AddMarker( mClock->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mClockChannel );
	mResults->AddMarker( mCommand->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mCommandChannel );
    mResults->CommitResults();

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
