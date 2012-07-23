#include "Audio_monitor.h"
#include "../MsTimer2/MsTimer2.h"

//-----------------------------------------------------

Audio_monitor Audio_monitor::singleton = Audio_monitor();

Audio_monitor::Audio_monitor()
: amp_cnt(0), sum(0), mean_amp_cnt(0), mean_sum(0), initializing(true), stdev(0), lock(false), serial_debug(false)//, amp(0), last_reading(0)
{
  // for Shifty
  pinMode(datapin, OUTPUT);
  pinMode(latchpin, OUTPUT);
  pinMode(enablepin, OUTPUT);
  pinMode(clockpin, OUTPUT);
  SPCR = (1<<SPE)|(1<<MSTR)|(0<<SPR1)|(1<<SPR0);
  digitalWrite(latchpin, LOW);
  digitalWrite(enablepin, LOW);
  analogReference(INTERNAL);

  memset(amplitudes, 0, sizeof(amplitudes));
  memset(recent_mean_amplitudes, 0, sizeof(recent_mean_amplitudes));
  
  /// Configure and start the interrupt that will call update_amplitude every SAMPLE_INTERVAL ms
  MsTimer2::set(SAMPLE_INTERVAL, &update_amplitude );
  MsTimer2::start();
}

//-----------------------------------------------------

Audio_monitor::~Audio_monitor()
{
  MsTimer2::stop();
}

//-----------------------------------------------------

Audio_monitor& Audio_monitor::instance()
{
  return singleton;
}

//-----------------------------------------------------

int Audio_monitor::get_amplitude() const
{
  unsigned int amp = round( sum / AMP_SIZE );
  if ( serial_debug )
  {
    //Serial.println( amp );
  }
  return amp;
}

//-----------------------------------------------------

int Audio_monitor::get_recent_amplitude_mean() const
{
  return initializing ? round( mean_sum / (mean_amp_cnt+1) ) : round( mean_sum / AMP_SIZE );
}

//-----------------------------------------------------

int Audio_monitor::get_amplitude_level( byte bins ) const
{
  return map(get_amplitude(),0,MAX_AMPLITUDE,0,bins-1);
}

//-----------------------------------------------------

void Audio_monitor::update( unsigned int latest_value )
{
  // need to keep this fast to ensure it doesn't miss samples. currently it isn't
  // missing any with 1ms sampling rate.
  if ( !lock )
  {
    lock = true;
    latest_value > MAX_AMPLITUDE ? MAX_AMPLITUDE : latest_value;
    update_array_and_sum( amplitudes, AMP_SIZE, sum, amp_cnt, latest_value );
    if ( amp_cnt == AMP_SIZE-1 )
    {
      update_array_and_sum( recent_mean_amplitudes, AMP_SIZE, mean_sum, mean_amp_cnt, get_amplitude() );
      if ( mean_amp_cnt == AMP_SIZE-1 && initializing )
      {
        initializing = false;
      }
      // Calculate standard deviation - slowest part
      long sum = 0;
      int mean = get_recent_amplitude_mean();
      for ( byte i = 0; i < AMP_SIZE; ++i )
      {
        sum += pow( (int)recent_mean_amplitudes[i] - mean, 2 );
      }
      stdev = sqrt( sum / (AMP_SIZE - 1) );
    }
    if ( debounce_counter > 0 )
    {
      --debounce_counter;
    }
    lock = false;
  }
  else 
  {
    if ( serial_debug )
    {
      Serial.print("BALLS! You missed a sample!\n");
    }
  }
}

//-----------------------------------------------------

void Audio_monitor::update_array_and_sum( unsigned int array[], const unsigned int array_size, long& sum, byte& counter, unsigned int new_value )
{
  /// Store the latest value in the array and also update the sum
  /// so that we don't have to recalculate it every time we want to calc the average
  sum -= array[counter];
  array[counter] = new_value;
  sum += new_value;
  ++counter;
  if ( counter >= array_size )
  {
    counter = 0;
  }
}

//-----------------------------------------------------

void Audio_monitor::update_amplitude()
{
  singleton.update( analogRead(audio_read_pin) * SENSITIVITY_MULTIPLIER );
}

//-----------------------------------------------------

bool Audio_monitor::is_anomolously_loud()
{
  /// samples 3 standard deviations above mean should only occur ~.1% of the time if the
  /// amplitudes are normally distributed
  bool rval = !initializing && !debounce_counter && ( get_amplitude() > get_recent_amplitude_mean() + stdev*3 );
  if ( rval )
  {
    debounce_counter = DEBOUNCE_INTERVAL / SAMPLE_INTERVAL;
  }
  return rval;
}

//-----------------------------------------------------

void Audio_monitor::test( bool verbose, bool reallyverbose )
{
  serial_debug = reallyverbose;
  if ( verbose )
  {
    if ( amp_cnt > 0 ) {
    Serial.print( "Most recent value in amplitudes array: " + String(amplitudes[amp_cnt-1]) + ", avg=" + String(get_amplitude()) + ", cnt=" + String(amp_cnt) + "\n" ); }
    if ( mean_amp_cnt > 0 ) {
    Serial.print( "Most recent value in mean amplitudes array: " + String(recent_mean_amplitudes[mean_amp_cnt-1]) + ", avg=" + String(get_recent_amplitude_mean()) + ", stdev=" + String(stdev) + ", cnt=" + String(mean_amp_cnt) + "\n" ); }
  }
  if ( is_anomolously_loud() )
  {
    Serial.print( "EEK!\n" );
  }
}
