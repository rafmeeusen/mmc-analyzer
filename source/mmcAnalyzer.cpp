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

bool mmcAnalyzer::GetCommandBit() {
    U64 risingedge_sample = mClock->GetSampleNumber();
    mCommand->AdvanceToAbsPosition( risingedge_sample ) ;
    U64 cmdvalue = mCommand->GetBitState();
    mClock->AdvanceToNextEdge();
    mClock->AdvanceToNextEdge();

    return ( cmdvalue == BIT_HIGH );
}

// find start bit, and put nrbits in RawBits struct; nothing more
RawBits mmcAnalyzer::GetBits(U16 nrbits)
{
    RawBits rb;

    // find start bit i.e. first LOW: 
    bool tmpbit; 
    do { 
        tmpbit = GetCommandBit();    
    } while (tmpbit);
    rb.bits.push_back(tmpbit);
    rb.samples.push_back(mCommand->GetSampleNumber());

    // remaining bits
    U16 remainingbits = nrbits - 1 ;
    for ( int i=0; i<remainingbits; i++ ) {
        tmpbit = GetCommandBit();
        rb.bits.push_back(tmpbit);
        rb.samples.push_back(mCommand->GetSampleNumber());
    }

    return rb;
}

PacketType mmcAnalyzer::SMgetExpected() {
    return mmcAnalyzer::nextExpected;
}

void mmcAnalyzer::SMinit() {
    mmcAnalyzer::nextExpected.dir = true; 
    mmcAnalyzer::nextExpected.bitlen = MMC_CMD_BITLEN;
    mmcAnalyzer::nextExpected.isCID = false;
    mmcAnalyzer::nextExpected.isCSD = false;
}

// state machine logic: what packet size is expected next depending on currently parsed packet?
// IN: pr = actual parse result
// OUT: mmcAnalyzer::nextExpected variable
void mmcAnalyzer::SMputActual( ParseResult pr ) {
    mmcAnalyzer::nextExpected.isCID = false;
    mmcAnalyzer::nextExpected.isCSD = false;
    if ( pr.dir ) {
        // got a CMD
        U16 cmdidx = pr.frames[0].mData1;
        if ( cmdidx == 0 ) {
            mmcAnalyzer::nextExpected.dir = true; // cmd
            mmcAnalyzer::nextExpected.bitlen = MMC_CMD_BITLEN; 
        } else if ( cmdidx == 2 ) {
            mmcAnalyzer::nextExpected.dir = false; // rsp
            mmcAnalyzer::nextExpected.bitlen = MMC_RSP2_BITLEN; 
            mmcAnalyzer::nextExpected.isCID = true;
        } else if ( cmdidx == 9 ) {
            mmcAnalyzer::nextExpected.dir = false; // rsp
            mmcAnalyzer::nextExpected.bitlen = MMC_RSP2_BITLEN; 
            mmcAnalyzer::nextExpected.isCSD = true;
        } else {
            mmcAnalyzer::nextExpected.dir = false; // rsp
            mmcAnalyzer::nextExpected.bitlen = MMC_RSP_DEFAULT_BITLEN; 
        }
    } else {
        // got a RSP
        mmcAnalyzer::nextExpected.dir = true; // cmd
        mmcAnalyzer::nextExpected.bitlen = MMC_CMD_BITLEN; 
    }
}


// 1) check validity: direction OK?
ParseResult mmcAnalyzer::Parse(PacketType pt, RawBits rb)
{
    ParseResult pr;
    pr.isOK = true;
    pr.dir = rb.bits[1];

// check for obvious protocol errors
// TODO: what to do in case of errors? immediately return pr??
    if ( pr.dir != pt.dir ) {
        pr.isOK = false;
        // ERROR! 
        // return pr;
    }
    // endbit always 1 ; OK?
    bool endbit = rb.bits.back();
    if ( ! endbit ) {
        // ERROR! 
        pr.isOK = false;
        // return pr;
    }

    // store fixed bit locations
    pr.startbit = rb.samples[0];
    pr.transmissionbit = rb.samples[1];
    pr.endbit = rb.samples.back();

    // parse other bits according to packet type
    vector<FrameBitBoundaries> cmdboundaries;

    // FrameBitBoundaries cmdboundaries;
    if ( pt.bitlen == MMC_RSP2_BITLEN ) {
        if ( pt.isCID ) {
            // 120 CID bits = 15 CID bytes (without CRC)
            cmdboundaries.push_back( {2,7} );            // 6 check bits
            cmdboundaries.push_back( {8, (8+24-1)} );    // 3 CID bytes
            cmdboundaries.push_back( {32,(32+48-1)} );   // 6 CID bytes ~ 48 bits PNM
            cmdboundaries.push_back( {80,(80+48-1)} );   // 6 CID bytes 
            cmdboundaries.push_back( {128, 134} );       // 7 CRC bits part of CID
        } else {
            // assuming isCSD ; also 15 bytes
            cmdboundaries.push_back( {2,7} );            // 6 check bits
            cmdboundaries.push_back( {8, (8+32-1)} );    // 4 CSD bytes
            cmdboundaries.push_back( {40, (40+16-1)} );  // 2 CSD bytes
            cmdboundaries.push_back( {56, (56+24-1)} );  // 3 CSD bytes
            cmdboundaries.push_back( {80, (80+24-1)} );  // 3 CSD bytes
            cmdboundaries.push_back( {104,(104+24-1)} ); // 3 CSD bytes
            cmdboundaries.push_back( {128, 134} );       // 7 CRC bits part of CSD
        }
 
    } else {
        cmdboundaries.push_back( {2,7} );   // 6 bits cmdidx
        cmdboundaries.push_back( {8,39} );  // 32 bits cmdarg 
        cmdboundaries.push_back( {40,46} ); // 7 bits crc
    }

    Frame tmp;
    U16 nrbits = 0;
    U64 data = 0;
    U64 mask = 0;
    for ( FrameBitBoundaries b : cmdboundaries ) {
        tmp.mFlags = 0;
        tmp.mStartingSampleInclusive =  rb.samples[b.firstbitidx];
        tmp.mEndingSampleInclusive = rb.samples[b.lastbitidx];
         
        nrbits = b.lastbitidx - b.firstbitidx + 1;
        mask =  1LL << (nrbits-1); 
        data = 0;
        for ( U16 bitidx = b.firstbitidx; bitidx <= b.lastbitidx; bitidx++ ) {
            if ( rb.bits[bitidx] ) {
                data |= mask;
            }
            mask = mask >> 1;
        }
        tmp.mData1 = data;
        pr.frames.push_back(tmp);
    }

    return pr;
}

// just put frames to analyzer results
void mmcAnalyzer::PutResults( ParseResult pr )
{
    mResults->AddMarker( pr.startbit, AnalyzerResults::Start, mSettings->mCommandChannel );
    if ( pr.dir ) {
        // mark CMD with X, RSP unmarked
        mResults->AddMarker( pr.transmissionbit, AnalyzerResults::X, mSettings->mCommandChannel );
    }

// { Dot, ErrorDot, Square, ErrorSquare, UpArrow, DownArrow, X, ErrorX, Start, Stop, One, Zero }
    for ( Frame f : pr.frames ) {
        mResults->AddFrame( f);
    }
    mResults->AddMarker( pr.endbit, AnalyzerResults::Stop, mSettings->mCommandChannel);
    mResults->CommitResults();
    ReportProgress( pr.endbit );
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

    PacketType pt;
    RawBits rb;
    ParseResult pr;

    for(;;) {
        pt = SMgetExpected();
        rb = GetBits(pt.bitlen);
        pr = Parse(pt, rb);
        PutResults( pr );
        SMputActual( pr );
    }

/*
TODO: introduce protocol error in above for loop
        if ( ! pr.isOK ) {
            // todo: protocol error
//            break; // just stop parsing for now ; maybe later try to recover
        }
        
*/
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
