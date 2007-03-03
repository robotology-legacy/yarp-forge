// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2007 Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#include <stdio.h>

#include <yarp/os/all.h>
#include <yarp/sig/all.h>

#include <deque>
#include <vector>

using namespace yarp::os;
using namespace yarp::sig;
using namespace std;


class Salience {
public:
    virtual void apply(ImageOf<PixelRgb>& src, 
                       ImageOf<PixelRgb>& dest,
                       ImageOf<PixelFloat>& sal) {
    }
};


// following Laurent Itti, Brian Scassellati.
class ColorSalience {
public:
    void apply(ImageOf<PixelRgb>& src, 
               ImageOf<PixelRgb>& dest,
               ImageOf<PixelFloat>& sal) {
        dest.resize(src);
        sal.resize(src);
        IMGFOR(src,x,y) {
            PixelRgb pix = src(x,y);
            float lum3 = (pix.r+pix.g+pix.b);
            if (lum3/3.0<60) {
                pix.r = pix.g = pix.b = 0;
            }
            float rn = 255.0 * pix.r / lum3;
            float gn = 255.0 * pix.g / lum3;
            float bn = 255.0 * pix.g / lum3;
            // in theory, apparently, yellow should be (rn+gn)/2 ...
            float yw = (rn+gn)-fabs(rn-gn)-bn;
            yw *= (pix.r>1.5*pix.b);
            yw *= (pix.g>1.5*pix.b);
            float s = 0;
            float r = rn - (gn+bn)/2;
            float g = gn - (rn+bn)/2;
            float b = bn - (rn+gn)/2;
            if (r>s) s = r;
            if (g>s) s = g;
            if (yw>s) s = yw;
            if (b>s) s = b;
            if (s>255) s = 255;
            if (s<0) s = 0;
            if (r<0) r = 0;
            if (g<0) g = 0;
            if (b<0) b = 0;
            if (r>255) r = 255;
            if (g>255) g = 255;
            if (b>255) b = 255;
            sal(x,y) = (unsigned char)s;
            dest(x,y) = PixelRgb((unsigned char)r,
                                 (unsigned char)g,
                                 (unsigned char)b);
        }
        //dest.copy(sal);
    }
};


class MotionSalience {
private:
    deque<ImageOf<PixelRgb> > history;
    int targetLen;
public:
    MotionSalience() {
        targetLen = 10;
    }

    void apply(ImageOf<PixelRgb>& src, 
               ImageOf<PixelRgb>& dest,
               ImageOf<PixelFloat>& sal) {
        dest.resize(src);
        sal.resize(src);
        sal.copy(src);

        history.push_back(src);

        ImageOf<PixelRgb>& past = history.front();

        IMGFOR(src,x,y) {
            PixelRgb& pix1 = src(x,y);
            PixelRgb& pix0 = past(x,y);
            float dr = pix0.r-pix1.r;
            float dg = pix0.g-pix1.g;
            float db = pix0.b-pix1.b;
            float v = dr*dr + dg*dg + db*db;
            float v2 = v*10;
            float v3 = v/10;
            if (v>255) { v = 255; }
            sal(x,y) = 1;
            PixelRgb& pixd = dest(x,y);
            if (v2>255) { v2 = 255; }
            if (v3>255) { v3 = 255; }
            pixd.r = (unsigned char)v;
            pixd.g = (unsigned char)v2;
            pixd.b = (unsigned char)v3;
        }

        if (history.size()>targetLen) {
            history.pop_front();
        }
    }
};


class RuddySalience {
private:
    float transformWeights[6];
    float transformDelta;
public:
    RuddySalience() {
        transformWeights[0] = 0.21742;
        transformWeights[1] = -0.36386;
        transformWeights[2] = 0.90572;
        transformWeights[3] = 0.00096756;
        transformWeights[4] = -0.00050073;
        transformWeights[5] = -0.00287;
        transformDelta = -50.1255;
    }

    float eval(PixelRgb& pix) {
        float r = pix.r;
        float g = pix.g;
        float b = pix.b;
        bool mask = (r>g) && (r<3*g) && (r>0.9*b) && (r<3*b) && (r>70);
        float judge = 0;
        if (mask) {
            float r2 = r*r;
            float g2 = g*g;
            float b2 = b*b;
            judge = 
                r*transformWeights[0] +
                g*transformWeights[1] +
                b*transformWeights[2] +
                r2*transformWeights[3] +
                g2*transformWeights[4] +
                b2*transformWeights[5] +
                transformDelta;
            judge *= 1.5;
	    }
        if (judge<0) judge = 0;
        if (judge>255) judge = 255;
        return judge;
    }

    void apply(ImageOf<PixelRgb>& src, 
               ImageOf<PixelRgb>& dest,
               ImageOf<PixelFloat>& sal) {
        dest.resize(src);
        sal.resize(src);
        IMGFOR(src,x,y) {
            PixelRgb pix = src(x,y);
            sal(x,y) = eval(pix);
        }
        dest.copy(sal);
    }
};


class GroupSalience {
public:
};


int main(int argc, char *argv[]) {
    BufferedPort<ImageOf<PixelRgb> > imgPort;
    ImageOf<PixelRgb> result;
    ImageOf<PixelFloat> sal;
    imgPort.open("/attn");
    Network::connect("/fakebot/camera","/attn");
    Network::connect("/attn","/v2");
    MotionSalience ms;
    ColorSalience cs;
    RuddySalience rs;
    while (true) {
        ImageOf<PixelRgb> *img = imgPort.read();
        rs.apply(*img,imgPort.prepare(),sal);
        imgPort.write();
    }
    return 0;
}

