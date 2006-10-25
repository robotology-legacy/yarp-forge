
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include <fstream>
using namespace std;

#include "listen.h"

#include "fft.h"

#include "yarpy.h"

#include <yarp/os/BufferedPort.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Network.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Sound.h>
#include <yarp/sig/SoundFile.h>

#include <fstream>

#define MAX_LEN 10000

//#define SAMPLE_RATE	22000
#define SAMPLE_RATE	19750

using namespace yarp::os;
using namespace yarp::sig;
using namespace std;

Semaphore moveOn(0);
Semaphore saveMutex(1);
double saveTime = -1000;
double saveModeTime = -1000;
Bottle saveBottle;
int modeStartStop = 0;
int saveSkip = 0;
Port siggy;


#define UNINTERPRETED

#define VOL 4

class ExtCmd : public BufferedPort<Bottle> {
public:
  Semaphore mutex;
  BufferedPort<Sound> pout;

  ExtCmd() : mutex(0) {
    useCallback();
  }

  virtual void onRead(Bottle& bot) {
    if (bot.size()==3) {
      printf("got a bottle: %s\n", bot.toString().c_str());
      setParamTarget(bot.get(0).asInt(), 
		     bot.get(1).asInt(), 
		     bot.get(2).asDouble());
    } else {
      int v = bot.get(0).asInt();
      if (v==999||v==998||v==997) {
	printf("got a large bottle: %s\n", bot.toString().c_str());
	for (int i=1; i<bot.size(); i+=3) {
	  setParamTarget(bot.get(i+0).asInt(), 
			 bot.get(i+1).asInt(), 
			 bot.get(i+2).asDouble());
	}
	if (v==998||v==997) {
	  //printf("should save\n");
	  saveMutex.wait();
	  saveTime = Time::now();
	  saveModeTime = saveTime;
	  saveBottle = bot;
	  saveMutex.post();
	  moveOn.post();
	}
      }
    }
  }
};

ExtCmd *extCmd = NULL;

void init_listen(const char *name) {
  Network::init();
  printf("Opening ports ...\n");
  extCmd = new ExtCmd;
  char buf[1000];
  sprintf(buf,"%s/audio", name);
  extCmd->pout.open(buf);
  sprintf(buf,"%s/cmd", name);
  extCmd->open(buf);
  sprintf(buf,"%s/sig", name);
  siggy.open(buf);
  printf("... ports opened\n");
  extCmd->mutex.post();
}


void port_audio(unsigned char *sample, int len) {
  int16_t *sample16 = (int16_t*)sample;
  extCmd->mutex.wait();
  Sound& sound = extCmd->pout.prepare();
  sound.resize(len,1);
  sound.setFrequency(SAMPLE_RATE);
  static int ct = 0;
  ct++;
  for (int i=0; i<len; i++) {
    sound.set(sample16[i]*VOL,i);
    //sound.set(i+ct%10,i);
  }

  double now = Time::now();
  saveMutex.wait();
  if (now-saveTime<1) {
    saveSkip++;
    if (saveSkip>=5) {
      static int ct = 0;
      char buf[256];
      char prefix[] = "/scratch/articulate";
      sprintf(buf,"%s/log-%09d.wav",prefix,ct);
      printf("Saving to %s\n",buf);
      yarp::sig::file::write(sound,buf);
      sprintf(buf,"%s/log-%09d.txt",prefix,ct);
      ofstream fout(buf);
      fout << saveBottle.toString().c_str() << endl;
      fout.close();
      ct++;
      saveTime = 0;
      saveSkip = 0;
      Bottle ack;
      ack.addInt(1);
      siggy.write(ack);
      saveMutex.post();
      printf("Waiting for next command...\n");
      moveOn.wait();
      saveMutex.wait();
    }
  }
  saveMutex.post();

  extCmd->pout.write(true);  //strict write
  extCmd->mutex.post();
  

  if (now - saveModeTime>10) {
    static double first = now;
    static double target = 0;
    target += ((double)len)/SAMPLE_RATE;
    double dt = target-(now-first);
    if (dt>0) {
      Time::delay(dt);
      //printf("wait for %g\n", dt);
    }
  }
}


void plisten(unsigned char *sample, int len) {
  //printf("got %d entities\n",len);
  //printf("missing port_audio call\n");
  port_audio(sample,len);
  return;

  static ofstream fout("/tmp/snd.txt");
  static ofstream fout2("/tmp/mot.txt");


#ifdef UNINTERPRETED
  // don't run a local FFT; leave all that to external processing

  for (int i=0; i<len; i++) {
    fout << ((int)(sample[i])) << " ";
  }
  fout << endl;
  fout.flush();


#else


  static int base_len = len;
  static FFT fft(len);
  static FFT fft2(len/2);
  static FFT fft3(len/4);
  static float input[MAX_LEN];
  static float mag[MAX_LEN];
  static float phase[MAX_LEN];
  assert(len==base_len);
  assert(len<=MAX_LEN);
  float sum = 0;
  int p = 0;
  for (int i=0; i<len; i++) {
    input[i] = sample[i];
    mag[i] = 0;
    phase[i] = 0;
    sum += fabs((float)sample[i]);
    p = sample[i];
  }
  sum/=len;
  fft.apply(input,mag,phase);

  int out_len = len/2;
  float *out = mag;
  float *out2 = NULL;

  for (int i=0; i<out_len; i++) {
    sum += out[i];
  }
  for (int i=0; i<out_len; i++) {
    out[i] /= sum;
  }

  for (int i=1; i<len/2; i++) {
    fout << mag[i] << " ";
  }

  fft2.apply(mag,input,phase);
  out_len = len/16;
  out = input;
  out2 = phase;

#if 0
  sum = 0;
  for (int i=0; i<out_len; i++) {
    sum += out[i];
  }
  for (int i=0; i<out_len; i++) {
    out[i] /= sum;
  }

  fft3.apply(input,mag,phase);
  out_len = len/8;
  out = mag;
#endif

  for (int i=0; i<out_len; i++) {
    fout << out[i] << " ";
  }
  if (out2!=NULL) {
    for (int i=0; i<out_len; i++) {
      fout << out2[i] << " ";
    }
  }
  fout << endl;
  fout.flush();
#endif

  fout2 << getTime() << " ";
  fout2 << getParam(0,0) << " ";
  fout2 << getParam(0,1) << " ";
  fout2 << getParam(1,0) << " ";
  fout2 << getParam(2,0) << " ";
  fout2 << getParam(2,1) << " ";
  fout2 << getParam(2,2) << " ";
  fout2 << getParam(2,3) << " ";
  for (int i=0; i<TOTAL_REGIONS; i++) {
    fout2 << getParam(3,i) << " ";
  }
  fout2 << getParam(4,0);
  fout2 << endl;
  fout2.flush();
  //printf("sum %g\n", sum);

}

