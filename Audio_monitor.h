#ifndef Audio_monitor_h
#define Audio_monitor_h

#include "Arduino.h"


/// If this is defined, the class will configure its own interrupt timer.
/// If not, some external interrupt timer needs to call its update_amplitude()
/// method manually. This is because the MsTimer2 library only supports one
/// interrupt at a time (and we have no time to switch libraries).
#define AUDIO_INTERNAL_INTERRUPT


/// Singleton class that performs interrupt-driven sampling of the amplitude inputs from
/// a Shifty VU and maintains a running average. Also stores a buffer of recent average amplitudes
/// and reports whether it's anomolously loud relative to the variance of the recent averages.
class Audio_monitor
{
public:
  static const byte SAMPLE_INTERVAL = 30; // milliseconds
  static const uint16_t MAX_AMPLITUDE = 720; //the empirically measured max possible mic reading
  static const float SENSITIVITY_MULTIPLIER = 1; //inputs get multiplied by this
  
  /// minimum ms between true return values of is_anomolously_loud(). should be a multiple
  /// of sample_interval.
  static const int DEBOUNCE_INTERVAL = 1000; 

  /// Gets the one static instance of the class
  static Audio_monitor& instance();
  
  /// Returns the average of the last AMP_SIZE amplitude measurements
  int get_amplitude() const;
  
  /// Same as above but in the [0,1] range
  float get_amplitude_float() const { return (float)get_amplitude()/MAX_AMPLITUDE; }

  /// Returns the average of the last SAMPLE_MEANS averages of the amplitude array
  /// (so timescale is SAMPLES*SAMPLE_MEANS)
  int get_recent_amplitude_mean() const;

  /// Returns whether or not it is anomolously loud currently. "anomolously loud"
  /// is not yet well-defined.
  bool is_anomolously_loud();
  
  /// Splits the amplitude measurement range into bins and returns which bin the
  /// current average amplitude falls within
  int get_amplitude_level( byte bins ) const;

  /// Prints some diagnostic info to the serial console. You need to call Serial.begin before
  /// calling this.
  void test( bool verbose, bool reallyverbose=false );
  
  /// Interrupt callback (has to be static)
  static void update_amplitude();
  
private:
  static const uint16_t SAMPLES = 20;
  static const uint16_t SAMPLE_MEANS = 50;
  static const byte clockpin = 13;
  static const byte enablepin = 10;
  static const byte latchpin = 9;
  static const byte datapin = 11;
  static const byte audio_read_pin = 2;
  
  // each number in here is the average of one set of SAMPLES.
  // we have to cache all the values in order to calculate the standard deviation.
  uint16_t recent_mean_amplitudes[SAMPLE_MEANS];
  static Audio_monitor singleton;
  byte amp_cnt, mean_amp_cnt;
  long sum; // decaying sum of last SAMPLES measures
  long mean_sum; // simple sum of values in recent_mean_amplitudes
  uint16_t amp;
  uint16_t last_reading;
  uint16_t stdev;
  bool initializing; // set to true after one full cycle through mean amplitudes array
  bool lock;
  bool serial_debug;
  uint16_t debounce_counter; // used to ensure that is_anomolously_loud() doesn't return true too often
  
  /// Private so that no one can create additional instances
  Audio_monitor();
  
  // unimplemented
  Audio_monitor( const Audio_monitor& );
  Audio_monitor& operator=(const Audio_monitor&);
  
  ~Audio_monitor();

  /// Adds new_value to the position in array given by counter, updates sum of array, updates counter
  static void update_array_and_sum( uint16_t array[], const uint16_t array_size, long& sum, byte& counter, uint16_t new_value );

  /// Called by update_amplitude
  void update( uint16_t latest_value );
};



#endif
