//
//  Created by Yuya Hanai on 10/16/14.
//

#include "ofProtonect2.h"

#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/async_packet_processor.h>
#include <libfreenect2/rgb_packet_stream_parser.h>
#include <libfreenect2/depth_packet_stream_parser.h>
#include <libfreenect2/logger.h>

using namespace libfreenect2;

class PassThroughRgbPacketProcessor : public RgbPacketProcessor
{
public:
    PassThroughRgbPacketProcessor() : rgb_packet_frame(NULL) {}
    virtual ~PassThroughRgbPacketProcessor() {}
protected:
    Frame *rgb_packet_frame;
    
    virtual void process(const libfreenect2::RgbPacket &packet)
    {
        rgb_packet_frame = new Frame(packet.jpeg_buffer_length, 1, 1);
        std::copy(packet.jpeg_buffer, packet.jpeg_buffer + packet.jpeg_buffer_length, rgb_packet_frame->data);
        
        if(listener_->onNewFrame(Frame::Color, rgb_packet_frame))
        {
        }
        else
        {
            delete rgb_packet_frame;
        }
        rgb_packet_frame = NULL;

    }
};

//----------------------------------------------------------------


class ofPacketPipeline : public PacketPipeline
{
protected:
    RgbPacketStreamParser *rgb_parser_;
    DepthPacketStreamParser *depth_parser_;
    
    RgbPacketProcessor *rgb_processor_;
    BaseRgbPacketProcessor *async_rgb_processor_;
    DepthPacketProcessor *depth_processor_;
    BaseDepthPacketProcessor *async_depth_processor_;

public:
    ofPacketPipeline(int cldeviceindex);
    virtual ~ofPacketPipeline();
    
    virtual PacketParser *getRgbPacketParser() const;
    virtual PacketParser *getIrPacketParser() const;
    
    virtual RgbPacketProcessor *getRgbPacketProcessor() const;
    virtual DepthPacketProcessor *getDepthPacketProcessor() const;
};

//----------------------------------------------------------------

ofPacketPipeline::ofPacketPipeline(int cldeviceindex)
{
    rgb_parser_ = new RgbPacketStreamParser();
    depth_parser_ = new DepthPacketStreamParser();
    
    rgb_processor_ = new PassThroughRgbPacketProcessor();

    string binpath = ofToDataPath("");
    OpenCLDepthPacketProcessor* depth_processor = new OpenCLDepthPacketProcessor(cldeviceindex);
    
    libfreenect2::DepthPacketProcessor::Config config;
    config.MinDepth = 0.4f;
    config.MaxDepth = 10.0f;
    config.EnableBilateralFilter = true;
    config.EnableEdgeAwareFilter = false;
    depth_processor->setConfiguration(config);
    
    depth_processor_ = depth_processor;
    
    async_rgb_processor_ = new AsyncPacketProcessor<RgbPacket>(rgb_processor_);
    async_depth_processor_ = new AsyncPacketProcessor<DepthPacket>(depth_processor_);
    
    rgb_parser_->setPacketProcessor(async_rgb_processor_);
    depth_parser_->setPacketProcessor(async_depth_processor_);
}

ofPacketPipeline::~ofPacketPipeline()
{
    delete async_rgb_processor_;
    delete async_depth_processor_;
    delete rgb_processor_;
    delete depth_processor_;
    delete rgb_parser_;
    delete depth_parser_;
}

ofPacketPipeline::PacketParser *ofPacketPipeline::getRgbPacketParser() const
{
    return rgb_parser_;
}

ofPacketPipeline::PacketParser *ofPacketPipeline::getIrPacketParser() const
{
    return depth_parser_;
}

RgbPacketProcessor *ofPacketPipeline::getRgbPacketProcessor() const
{
    return rgb_processor_;
}

DepthPacketProcessor *ofPacketPipeline::getDepthPacketProcessor() const
{
    return depth_processor_;
}

//----------------------------------------------------------------
ofProtonect2::ofProtonect2()  :
dev(NULL), listener(NULL), bOpen(false), pipeline(NULL) {
}

bool ofProtonect2::open(const std::string &serial, int mode, int clindex)
{
    if (bOpen) {
        return false;
    }
    
    pipeline = new ofPacketPipeline(clindex);
    dev = freenect2.openDevice(serial, pipeline);
    
    if (dev == 0)
    {
        std::cout << "no device connected or failure opening the default one!" << std::endl;
        return false;
    }
    
    listener = new libfreenect2::SyncMultiFrameListener(mode);
    
    dev->setColorFrameListener(listener);
    dev->setIrAndDepthFrameListener(listener);

    bOpen = true;
    return true;
}


void ofProtonect2::start() {
    if (!bOpen) {
        return;
    }
    dev->start();
    
    std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
    std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;
}

void ofProtonect2::update() {
    if (!bOpen) {return;}
    listener->waitForNewFrame(frames);
    if (!bOpen) {return;}
    
    {
        libfreenect2::Frame *rgbFrame = frames[libfreenect2::Frame::Color];
        if (rgbFrame) {
            int sz = rgbFrame->width * rgbFrame->height * rgbFrame->bytes_per_pixel;
            jpeg.resize(sz);
            std::copy(rgbFrame->data, rgbFrame->data + sz, &jpeg.front());
        }
    }
    {
        libfreenect2::Frame *irFrame = frames[libfreenect2::Frame::Ir];
        if (irFrame) {
            int sz = irFrame->width * irFrame->height * irFrame->bytes_per_pixel;
            rawir.resize(sz);
            std::copy(irFrame->data, irFrame->data + sz, &rawir.front());
        }
    }
    {
        libfreenect2::Frame *depthFrame = frames[libfreenect2::Frame::Depth];
        if (depthFrame) {
            int sz = depthFrame->width * depthFrame->height * depthFrame->bytes_per_pixel;
            rawdepth.resize(sz);
            std::copy(depthFrame->data, depthFrame->data + sz, &rawdepth.front());
        }
    }
    
    listener->release(frames);
    
    if( ofGetLogLevel() == OF_LOG_VERBOSE ){
        libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
    }else{
        libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Warning));
    }
}

void ofProtonect2::close() {
    if (bOpen) {
        // TODO: restarting ir stream doesn't work!
        // TODO: bad things will happen, if frame listeners are freed before dev->stop() :(
        dev->stop();
        dev->close();
        pipeline = NULL;
    }
    bOpen = false;
}

