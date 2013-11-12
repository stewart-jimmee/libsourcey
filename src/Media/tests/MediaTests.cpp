#include "Sourcey/Application.h"
#include "Sourcey/Logger.h"
#include "Sourcey/Signal.h"
#include "Sourcey/Queue.h"
#include "Sourcey/PacketQueue.h"
//#include "Sourcey/SyncDelegate.h"
#include "Sourcey/Media/FLVMetadataInjector.h"
#include "Sourcey/Media/FormatRegistry.h"
#include "Sourcey/Media/MediaFactory.h"
#include "Sourcey/Media/AudioCapture.h"
#include "Sourcey/Media/AVPacketEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/fifo.h>
#include <libswscale/swscale.h>
}
/*
#include "Sourcey/Timer.h"
#include "Sourcey/Net/UDPSocket.h"
#include "Sourcey/Net/TCPSocket.h" //Client


#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#include <assert.h>
#include <stdio.h>


// Detect Memory Leaks
#ifdef _DEBUG
#include "MemLeakDetect/MemLeakDetect.h"
#include "MemLeakDetect/MemLeakDetect.cpp"
CMemLeakDetect memLeakDetect;
#endif
*/


using namespace std;
using namespace scy;
using namespace scy::av;
using namespace cv;


namespace scy {
namespace av {


class Tests
{		
public:
	Application& app;

	Tests(Application& app) : app(app)
	{   
		debugL() << "Running tests..." << endl;	

		// Set up our devices
		//MediaFactory::initialize();
		//MediaFactory::instance().loadVideo();

		try
		{			
			//MediaFactory::instance().devices().print(cout);
			//audioCapture = new AudioCapture(device.id, 2, 44100);
			
			//videoCapture = new VideoCapture(0, true);
			//audioCapture = new AudioCapture(0, 1, 16000);	
			//audioCapture = new AudioCapture(0, 1, 11025);	
			//for (int i = 0; i < 100; i++) { //0000000000
			//	runAudioCaptureTest();
			//}
			
			//testDeviceManager();
			//runAudioCaptureThreadTest();
			//runVideoCaptureThreadTest();
			//runAudioCaptureTest();
			//runOpenCVMJPEGTest();
			//runVideoRecorderTest();
			//runAudioRecorderTest();
			//runCaptureEncoderTest();
			//runAVEncoderTest();
			runStreamEncoderTest();
			//runMediaSocketTest();
			//runFormatFactoryTest();
			//runMotionDetectorTest();
			//runBuildXMLString();
		
			//runStreamEncoderTest();
			//runOpenCVCaptureTest();
			//runDirectShowCaptureTest();
		}
		catch (std::exception& exc)
		{
			errorL() << "Error: " << std::string(exc.what()) << endl;
		}		

		//MediaFactory::instance().unloadVideo();

		//if (videoCapture)
		//	delete audioCapture;
		//if (videoCapture)
		//	delete videoCapture;		

		debugL() << "Tests Exiting..." << endl;	
	};
	
		
	void testDeviceManager()
	{
		cout << "Starting" << endl;	

		Device device;
		if (MediaFactory::instance().devices().getDefaultVideoCaptureDevice(device)) {
			cout << "Default Video Device: " << device.id << ": " << device.name << endl;
		}
		if (MediaFactory::instance().devices().getDefaultAudioInputDevice(device)) {
			cout << "Default Audio Device: " << device.id << ": " << device.name << endl;
		}

		std::vector<Device> devices;
		if (MediaFactory::instance().devices().getVideoCaptureDevices(devices)) {			
			for (std::vector<Device>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
				cout << "Printing Video Device: " << (*it).id << ": " << (*it).name << endl;
			}
		}
		if (MediaFactory::instance().devices().getAudioInputDevices(devices)) {			
			for (std::vector<Device>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
				cout << "Printing Audio Device: " << (*it).id << ": " << (*it).name << endl;
			}
		}
	}


	// ---------------------------------------------------------------------
	// Packet Stream Encoder Test
	//
	static bool stopStreamEncoders;

	class StreamEncoderTest
	{
	public:
		StreamEncoderTest(const av::RecordingOptions& opts) : 
			closed(false), options(opts), frames(0), videoCapture(nullptr), audioCapture(nullptr)
		{
			//ofile.open("enctest1.mp4", ios::out | ios::binary);
			//assert(ofile.is_open());

			// Synchronize events and packet output with the default loop
			//stream.synchronizeOutput(uv::defaultLoop());
			//stream.setAsyncContext(std::make_shared<Idler>(uv::defaultLoop()));

			// Init captures
			if (options.oformat.video.enabled) {
				debugL("StreamEncoderTest") << "Video device: " << 0 << endl;
				videoCapture = MediaFactory::instance().createVideoCapture(0); //0
				//videoCapture = MediaFactory::instance().createFileCapture("D:/dev/lib/ffmpeg/bin/channel1.avi"); //0
				//videoCapture->emitter += packetDelegate(this, &StreamEncoderTest::onVideoCapture);
				videoCapture->getEncoderFormat(options.iformat);		
				stream.attachSource(videoCapture, true, true); //
				
				//options.iformat.video.pixelFmt = "yuv420p";
			}
			if (options.oformat.audio.enabled) {
				Device device;
				if (MediaFactory::instance().devices().getDefaultAudioInputDevice(device)) {
					debugL("StreamEncoderTest") << "Audio device: " << device.id << endl;
					audioCapture = MediaFactory::instance().createAudioCapture(device.id,
						options.oformat.audio.channels, 
						options.oformat.audio.sampleRate);
					stream.attachSource(audioCapture, true, true);
				}
				else assert(0);
			}			
								
			// Init as async queue for testing
			//stream.attach(new FPSLimiter(5), 4, true);
			//stream.attach(new AsyncPacketQueue, 2, true);
			//stream.attach(new AsyncPacketQueue, 3, true);
			//stream.attach(new AsyncPacketQueue, 4, true);

			// Init encoder				
			encoder = new AVPacketEncoder(options, 
				options.oformat.video.enabled && 
				options.oformat.audio.enabled);
			stream.attach(encoder, 5, true);

			// Start the stream
			stream.emitter += packetDelegate(this, &StreamEncoderTest::onVideoEncoded);	
			stream.base().StateChange += delegate(this, &StreamEncoderTest::onStreamStateChange);
			stream.start();
		}
		
		virtual ~StreamEncoderTest()
		{		
			debugL("StreamEncoderTest") << "Destroying" << endl;
			close();
			debugL("StreamEncoderTest") << "Destroying: OK" << endl;
		}

		void close()
		{
			debugL("StreamEncoderTest") << "########### Closing: " << frames << endl;
			closed = true;
			
			// Close the stream
			// This will flush any queued items
			//stream.stop();
			//stream.base().waitForSync();
			stream.close();
			
			// Make sure everything shutdown properly
			//assert(stream.base().queue().empty());
			//assert(encoder->isStopped());
			
			// Close the output file
			//ofile.close();
		}
			
		void onStreamStateChange(void* sender, PacketStreamState& state, const PacketStreamState& oldState)
		{
			debugL("StreamEncoderTest") << "########### On stream state change: " << oldState << " => " << state << endl;
		}

		void onVideoEncoded(void* sender, RawPacket& packet)
		{
			debugL("StreamEncoderTest") << "########### On packet: " << closed << ":" << packet.size() << endl;
			frames++;
			//assert(!closed);
			assert(packet.data());
			assert(packet.size());
			
			// Do not call stream::close from inside callback
			//ofile.write(packet.data(), packet.size());
			//assert(frames <= 3);
			//if (frames == 20)
			//	close();
		}
			
		void onVideoCapture(void* sender, av::MatrixPacket& packet)
		{
			debugL("StreamEncoderTest") << "VideoCaptureThread: On packet: " << packet.size() << endl;
			
			//cv::imshow("StreamEncoderTest", *packet.mat);
		}
		
		int frames;
		bool closed;
		PacketStream stream;
		VideoCapture* videoCapture;
		AudioCapture* audioCapture;
		//AsyncPacketQueue* queue;
		AVPacketEncoder* encoder;
		av::RecordingOptions options;
		//std::ofstream ofile;
	};	
				
	/*
	static void onShutdownSignal(void* opaque)
	{
		auto& tests = *reinterpret_cast<std::vector<StreamEncoderTest*>*>(opaque);

		stopStreamEncoders = true;

		for (unsigned i = 0; i < tests.size(); i++) {
			// Delete the pointer directly to 
			// ensure synchronization is golden.
			delete tests[i];
		}
	}
	*/

	void runStreamEncoderTest()
	{
		debugL("StreamEncoderTest") << "Running" << endl;	
		try
		{
			// Setup encoding format
			Format mp4(Format("MP4", "mp4", 
				VideoCodec("MPEG4", "mpeg4", 640, 480, 60), 
				//VideoCodec("H264", "libx264", 640, 480, 20)//,
				//AudioCodec("AAC", "aac", 2, 44100)
				//AudioCodec("MP3", "libmp3lame", 1, 8000, 64000)
				//AudioCodec("MP3", "libmp3lame", 2, 44100, 64000)
				AudioCodec("AC3", "ac3_fixed", 2, 44100, 64000)
			));

			Format mp3("MP3", "mp3", 
				AudioCodec("MP3", "libmp3lame", 1, 8000, 64000)); 
		
			//stopStreamEncoders = false;

			av::RecordingOptions options;		
			//options.ofile = "enctest.mp3";
			options.ofile = "itsanewday.mp4";	
			//options.ofile = "enctest.mjpeg";	
			options.oformat = mp4;

			// Initialize test runners
			int numTests = 1;
			std::vector<StreamEncoderTest*> threads;
			for (unsigned i = 0; i < numTests; i++) {
				threads.push_back(new StreamEncoderTest(options));
			}

			// Run until Ctrl-C is pressed
			//Application app;
			app.waitForShutdown(); //&threadsnullptr, nullptr
			
			for (unsigned i = 0; i < threads.size(); i++) {
				// Delete the pointer directly to 
				// ensure synchronization is golden.
				delete threads[i];
			}

			// Shutdown the garbage collector so we can free memory.
			//GarbageCollector::instance().shutdown();
			//debugL("Tests") << "#################### Finalizing" << endl;
			//GarbageCollector::instance().shutdown();
			//debugL("Tests") << "#################### Exiting" << endl;

			//debugL("Tests") << "#################### Finalizing" << endl;
			//app.cleanup();
			//debugL("Tests") << "#################### Exiting" << endl;

			// Wait for enter keypress
			//scy::pause();
		
			// Finalize the application to free all memory
			// Note: 2 tiny mem leaks (964 bytes) are from OpenCV
			//app.finalize();
		}
		catch (std::exception& exc)
		{
			errorL("StreamEncoderTest") << "Error: " << std::string(exc.what()) << endl;
			assert(0);
		}

		debugL("StreamEncoderTest") << "Ended" << endl;
	}
	

			// Attach a few adapters to better test destruction synchronization
			//stream.attach(new FPSLimiter(100), 6, true);
			//stream.attach(new FPSLimiter(100), 7, true);
			//stream.attach(new FPSLimiter(3), 4, true);
			// For best practice we would detach the delegate,
			// but we want to make sure there are no late packets 
			// arriving after the stream is closed.
			//stream.detach(packetDelegate(this, &StreamEncoderTest::onVideoEncoded));		
			/*
			// Manually flush queue
			while (!queue->_queue.empty()) {				
				int s = queue->_queue.size();
				//queue->run(); // flush pending
				debugL("StreamEncoderTest") << "########### Waiting for queue: " << s << endl;
				scy::sleep(50);
			}
			assert(queue->_queue.empty());
			queue->cancel();
			//queue->clear();

			// Close the encoder
			encoder->uninitialize();	
			*/	

			
	// ---------------------------------------------------------------------
	// Capture Encoder Test
	//		
	class CaptureEncoder {
	public:
		CaptureEncoder(ICapture* capture, const av::RecordingOptions& options) : 
			capture(capture), encoder(options), closed(false) {
			assert(capture);	
			encoder.initialize();
		};

		void start()
		{
			capture->emitter += packetDelegate(this, &CaptureEncoder::onFrame);	
			capture->start();
		}

		void stop()
		{
			capture->emitter -= packetDelegate(this, &CaptureEncoder::onFrame);	
			capture->stop();
			encoder.uninitialize();
			closed = true;
		}

		void onFrame(void* sender, RawPacket& packet)
		{
			debugL("CaptureEncoder") << "On packet: " << packet.size() << endl;
			assert(!closed);
			try 
			{	
				assert(0);
				//encoder.process(packet);
			}
			catch (std::exception& exc) 
			{
				errorL("CaptureEncoder") << "#######################: " << std::string(exc.what()) << endl;
				stop();
			}
		}

		bool closed;
		ICapture* capture;
		AVEncoder encoder;
	};


	void runCaptureEncoderTest()
	{
		debugL("CaptureEncoderTest") << "Starting" << endl;	
		
		/*
		av::VideoCapture capture(0);

		// Setup encoding format
		Format mp4(Format("MP4", "mp4", 
			VideoCodec("MPEG4", "mpeg4", 640, 480, 10)//, 
			//VideoCodec("H264", "libx264", 320, 240, 25),
			//AudioCodec("AAC", "aac", 2, 44100)
			//AudioCodec("MP3", "libmp3lame", 2, 44100, 64000)
			//AudioCodec("AC3", "ac3_fixed", 2, 44100, 64000)
		));
		
		av::RecordingOptions options;			
		options.ofile = "enctest.mp4"; // enctest.mjpeg
		options.oformat = mp4;
		setVideoCaptureInputFormat(&capture, options.iformat);	
		
		CaptureEncoder encoder(&capture, options);
		encoder.start();

		std::puts("Press any key to continue...");
		std::getchar();

		encoder.stop();
		*/

		debugL("CaptureEncoderTest") << "Complete" << endl;	
	}


	// ---------------------------------------------------------------------
	// OpenCV Capture Test
	//	
	void runOpenCVCaptureTest()
	{
		debugL("StreamEncoderTest") << "Starting" << endl;	

		cv::VideoCapture cap(0);
		if (!cap.isOpened())
			assert(false);

		cv::Mat edges;
		cv::namedWindow("edges",1);
		for(;;) {
			cv::Mat frame;
			cap >> frame; // get a new frame from camera
			cv::cvtColor(frame, edges, CV_BGR2GRAY);
			cv::GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
			cv::Canny(edges, edges, 0, 30, 3);
			cv::imshow("edges", edges);
			if (cv::waitKey(30) >= 0) break;
		}

		debugL("StreamEncoderTest") << "Complete" << endl;	
	}
	
	
		/*
	// ---------------------------------------------------------------------
	// Video Capture Thread Test
	//	
	class VideoCaptureThread: public basic::Runnable
	{
	public:
		VideoCaptureThread(int deviceID, const std::string& name = "Capture Thread") : 
			closed(false),
			_deviceID(deviceID),
			frames(0)
		{	
			_thread.setName(name);
			_thread.start(*this);
		}		

		VideoCaptureThread(const std::string& filename, const std::string& name = "Capture Thread") : 
			closed(false),
			_filename(filename),
			_deviceID(0),
			frames(0)
		{	
			_thread.setName(name);
			_thread.start(*this);
		}	
		
		~VideoCaptureThread()
		{
			closed = true;
			_thread.join();
		}

		void run()
		{
			try
			{				
				VideoCapture* capture = !_filename.empty() ?
					MediaFactory::instance().createFileCapture(_filename) : //, true
					MediaFactory::instance().getVideoCapture(_deviceID);
				capture->attach(packetDelegate(this, &VideoCaptureThread::onVideo));	
				capture->start();

				while (!closed)
				{
					cv::waitKey(50);
				}
				
				capture->detach(packetDelegate(this, &VideoCaptureThread::onVideo));	

				debugL("VideoCaptureThread") << " Ending.................." << endl;
			}
			catch (std::exception& exc)
			{
				Log("error") << "[VideoCaptureThread] Error: " << std::string(exc.what()) << endl;
			}
			
			debugL("VideoCaptureThread") << " Ended.................." << endl;
			//delete this;
		}

		void onVideo(void* sender, MatPacket& packet)
		{
			debugL() << "VideoCaptureThread: On Packet: " << packet.size() << endl;
			//debugL() << "VideoCaptureThread: On Packet 1: " << packet.mat->total() << endl;
			//debugL() << "VideoCaptureThread: On Packet 1: " << packet.mat->cols << endl;
			//assert(packet.mat->cols);
			//assert(packet.mat->rows);
			cv::imshow(_thread.name(), *packet.mat);
			frames++;
		}
		
		Thread _thread;
		std::string _filename;
		int	_deviceID;
		int frames;
		bool closed;
	};


	void runVideoCaptureThreadTest()
	{
		debugL() << "Running video capture test..." << endl;	
				
		// start and destroy multiple receivers
		list<VideoCaptureThread*> threads;
		for (int i = 0; i < 3; i++) { //0000000000			
			threads.push_back(new VideoCaptureThread(0)); //, Poco::format("Capture Thread %d", i)
			//threads.push_back(new VideoCaptureThread("D:\\dev\\lib\\ffmpeg-1.0.1-gpl\\bin\\output.avi", Poco::format("Capture Thread %d", i)));
		}

		//util::pause();

		util::clearList(threads);
	}
	
	
	// ---------------------------------------------------------------------
	// Audio Capture Thread Test
	//
	class AudioCaptureThread: public basic::Runnable
	{
	public:
		AudioCaptureThread(const std::string& name = "Capture Thread")
		{	
			_thread.setName(name);
			_thread.start(*this);
		}		
		
		~AudioCaptureThread()
		{
			_wakeUp.set();
			_thread.join();
		}

		void run()
		{
			try
			{
				//capture = new AudioCapture(0, 2, 44100);
				//capture = new AudioCapture(0, 1, 16000);	
				//capture = new AudioCapture(0, 1, 11025);	
				AudioCapture* capture = new AudioCapture(0, 1, 16000);
				capture->attach(audioDelegate(this, &AudioCaptureThread::onAudio));	
				
				_wakeUp.wait();
				
				capture->detach(audioDelegate(this, &AudioCaptureThread::onAudio));	
				delete capture;
				
				debugL() << "[AudioCaptureThread] Ending.................." << endl;
			}
			catch (std::exception& exc)
			{
				Log("error") << "[AudioCaptureThread] Error: " << std::string(exc.what()) << endl;
			}
			
			debugL() << "[AudioCaptureThread] Ended.................." << endl;
			//delete this;
		}

		void onAudio(void* sender, AudioPacket& packet)
		{
			debugL() << "[AudioCaptureThread] On Packet: " << packet.size() << endl;
			//cv::imshow(_thread.name(), *packet.mat);
		}
		
		Thread	_thread;
		Poco::Event		_wakeUp;
		int				frames;
	};


	void runAudioCaptureThreadTest()
	{
		debugL() << "Running Audio Capture Thread test..." << endl;	
				
		// start and destroy multiple receivers
		list<AudioCaptureThread*> threads;
		for (int i = 0; i < 10; i++) { //0000000000
			threads.push_back(new AudioCaptureThread()); //Poco::format("Audio Capture Thread %d", i))
		}

		//util::pause();

		util::clearList(threads);
	}

	
	// ---------------------------------------------------------------------
	// Audio Capture Test
	//
	void onCaptureTestAudioCapture(void*, AudioPacket& packet)
	{
		debugL() << "onCaptureTestAudioCapture: " << packet.size() << endl;
		//cv::imshow("Target", *packet.mat);
	}	

	void runAudioCaptureTest()
	{
		debugL() << "Running Audio Capture test..." << endl;	
		
		AudioCapture* capture = new AudioCapture(0, 1, 16000);
		capture->attach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));
		//Util::pause();
		capture->detach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));
		delete capture;

		AudioCapture* capture = new AudioCapture(0, 1, 16000);
		capture->attach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));	
		//audioCapture->start();
		
		//Util::pause();

		capture->detach(packetDelegate(this, &Tests::onCaptureTestAudioCapture));
		delete capture;
		//audioCapture->stop();
	}
		*/
	
		/*
		//tests[i]->close();
		//debugL("StreamEncoderTest") << "########### Closing: " 
		//	<< i << ": " << tests[i]->frames << ": " << tests[i]->closed << endl;
		//assert(tests[i]->closed);
		// start and destroy multiple receivers
		list<StreamEncoderTest*> threads;
		for (unsigned i = 0; i < 1; i++) { //0000000000
			//Log("debug") << "StreamEncoderTest: " << _name << endl;
			threads.push_back(new StreamEncoderTest);
			//StreamEncoderTest test;
			//UDPSocket sock;
		}
		//util::pause();
		debugL() << "Stream Encoder Test: Cleanup" << endl;	
		util::clearList(threads);
		thread.run();
		*/
	
				/*
				options.oformat = Format("MJPEG High", "mjpeg", VideoCodec("mjpeg", "mjpeg", 640, 480, 10));		
				options.oformat.video.pixelFmt = "yuvj420p";
				options.oformat.video.quality = 100;

				//options.oformat = Format("MP4", "mpeg4",
					//VideoCodec("MPEG4", "mpeg4", 640, 480, 25), 
					//VideoCodec("H264", "libx264", 320, 240, 25),
					//AudioCodec("AC3", "ac3_fixed", 2, 44100));
			
				//options.oformat  = Format("MP3", "mp3", 
				//	VideoCodec(), 
				//	AudioCodec("MP3", "libmp3lame", 1, 8000, 64000)); //)

				//options.oformat = Format("MP4", Format::MP4, VideoCodec(Codec::MPEG4, "MPEG4", 1024, 768, 25));
				//options.oformat = Format("MJPEG", "mjpeg", VideoCodec(Codec::MJPEG, "MJPEG", 1024, 768, 25));
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));	
				//options.oformat = Format("FLV", "flv", 
				//	VideoCodec(Codec::FLV, "FLV", 400, 300, 15)//,
				//	//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 11025)
				//	);
				//options.ofile = "av_capture_test.mp4";
				*/
	
	/*
	// ---------------------------------------------------------------------
	//
	// Video Media Socket Test
	//
	// ---------------------------------------------------------------------	

	class MediaConnection: public TCPServerConnection
	{
	public:
		MediaConnection(const StreamSocket& s) : 
		  TCPServerConnection(s), stop(false)//, lastTimestamp(0), timestampGap(0), waitForKeyFrame(true)
		{		
		}
		
		void run()
		{
			try
			{
				av::RecordingOptions options;
				//options.ofile = "enctest.mp4";
				//options.stopAt = time(0) + 3;
				av::setVideoCaptureInputForma(videoCapture, options.iformat);	
				//options.oformat = Format("MJPEG", "mjpeg", VideoCodec(Codec::MJPEG, "MJPEG", 1024, 768, 25));
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));	
				options.oformat = Format("FLV", "flv", VideoCodec(Codec::FLV, "FLV", 640, 480, 100));	
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::FLV, "FLV", 320, 240, 15));	
				//options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));		
					

				//options.iformat.video.pixfmt = (scy::av::PixelFormat::ID)PIX_FMT_GRAY8; //PIX_FMT_BGR8; //PIX_FMT_BGR32 // PIX_FMT_BGR32
				//MotionDetector* detector = new MotionDetector();
				//detector->setVideoCapture(videoCapture);
				//stream.attach(detector, true);		
				//stream.attach(new SurveillanceMJPEGPacketizer(*detector), 20, true);	

				stream.attach(videoCapture, false);

				stream.attach(packetDelegate(this, &MediaConnection::onVideoEncoded));
		
				// init encoder				
				AVEncoder* encoder = new AVEncoder();
				encoder->setParams(options);
				encoder->initialize();
				stream.attach(encoder, 5, true);				
				
				//HTTPMultipartAdapter* packetizer = new HTTPMultipartAdapter("image/jpeg");
				//stream.attach(packetizer);

				//FLVMetadataInjector* injector = new FLVMetadataInjector(options.oformat);
				//stream.attach(injector);

				// start the stream
				stream.start();

				while (!stop)
				{
					Thread::sleep(50);
				}
				
				//stream.detach(packetDelegate(this, &MediaConnection::onVideoEncoded));
				//stream.stop();

				//outputFile.close();
				cerr << "MediaConnection: ENDING!!!!!!!!!!!!!" << endl;
			}
			catch (std::exception& exc)
			{
				cerr << "MediaConnection: " << std::string(exc.what()) << endl;
			}
		}

		void onVideoEncoded(void* sender, RawPacket& packet)
		{
			StreamSocket& ss = socket();

			fpsCounter.tick();
			debugL() << "On Video Packet Encoded: " << fpsCounter.fps << endl;

			//if (fpsCounter.frames < 10)
			//	return;
			//if (fpsCounter.frames == 10) {
			//	stream.reset();
			//	return;
			//}

			try
			{		
				ss.sendBytes(packet.data, packet.size);
			}
			catch (std::exception& exc)
			{
				cerr << "MediaConnection: " << std::string(exc.what()) << endl;
				stop = true;
			}
		}
		
		bool stop;
		PacketStream stream;
		FPSCounter fpsCounter;
	};

	void runMediaSocketTest()
	{		
		debugL() << "Running Media Socket Test" << endl;
		
		ServerSocket svs(666);
		TCPServer srv(new TCPServerConnectionFactoryImpl<MediaConnection>(), svs);
		srv.start();
		//util::pause();
	}

	
	// ---------------------------------------------------------------------
	//
	// Video CaptureEncoder Test
	//
	// ---------------------------------------------------------------------
	//UDPSocket outputSocket;

	void runCaptureEncoderTest()
	{		
		debugL() << "Running Capture Encoder Test" << endl;	

		av::RecordingOptions options;
		options.ofile = "enctest.mp4";
		//options.stopAt = time(0) + 3;
		av::setVideoCaptureInputForma(videoCapture, options.iformat);	
		//options.oformat = Format("MJPEG", "mjpeg", VideoCodec(Codec::MJPEG, "MJPEG", 400, 300));
		options.oformat = Format("FLV", "flv", VideoCodec(Codec::H264, "H264", 400, 300, 25));	
		//options.oformat = Format("FLV", "flv", VideoCodec(Codec::FLV, "FLV", 320, 240, 15));	
				
		//CaptureEncoder<VideoEncoder> enc(videoCapture, options);	
		//encoder = new AVEncoder(stream.options());
		CaptureRecorder enc(options, videoCapture, audioCapture);		
		//enc.initialize();	
		
		enc.attach(packetDelegate(this, &Tests::onCaptureEncoderTestVideoEncoded));
		enc.start();
		//util::pause();
		enc.stop();

		debugL() << "Running Capture Encoder Test: END" << endl;	
	}
	
	FPSCounter fpsCounter;
	void onCaptureEncoderTestVideoEncoded(void* sender, MediaPacket& packet)
	{
		fpsCounter.tick();
		debugL() << "On Video Packet Encoded: " << fpsCounter.fps << endl;
	}

	// ---------------------------------------------------------------------
	//
	// Video CaptureRecorder Test
	//
	// ---------------------------------------------------------------------
	void runVideoRecorderTest()
	{
		av::RecordingOptions options;
		options.ofile = "av_capture_test.flv";
		
		options.oformat = Format("FLV", "flv", 
			VideoCodec(Codec::FLV, "FLV", 320, 240, 15),
			//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 11025),
			AudioCodec(Codec::Speex, "Speex", 1, 16000)//,
			//AudioCodec(Codec::Speex, "Speex", 2, 44100)
			);	
		//options.oformat = Format("MP4", Format::MP4,
		options.oformat = Format("FLV", "flv",
			//VideoCodec(Codec::MPEG4, "MPEG4", 640, 480, 25), 
			//VideoCodec(Codec::H264, "H264", 640, 480, 25), 
			VideoCodec(Codec::FLV, "FLV", 640, 480, 25), 
			//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 11025)
			//AudioCodec(Codec::Speex, "Speex", 2, 44100)
			//AudioCodec(Codec::MP3, "MP3", 2, 44100)
			//AudioCodec(Codec::AAC, "AAC", 2, 44100)
			AudioCodec(Codec::AAC, "AAC", 1, 11025)
		);

		options.oformat = Format("M4A", Format::M4A,
			//AudioCodec(Codec::NellyMoser, "NellyMoser", 1, 44100)
			AudioCodec(Codec::AAC, "AAC", 2, 44100)
			//AudioCodec(Codec::AC3, "AC3", 2, 44100)
		);



		//options.stopAt = time(0) + 5; // Max 24 hours		
		av::setVideoCaptureInputForma(videoCapture, options.iformat);	

		CaptureRecorder enc(options, nullptr, audioCapture); //videoCapture
			
		audioCapture->start();	
		enc.start();
		//util::pause();
		enc.stop();
	}

	// ---------------------------------------------------------------------
	//
	// Audio CaptureRecorder Test
	//
	// ---------------------------------------------------------------------
	void runAudioRecorderTest()
	{
		av::RecordingOptions options;
		options.ofile = "audio_test.mp3";
		options.stopAt = time(0) + 5;
		options.oformat = Format("MP3", "mp3",
			AudioCodec(Codec::MP3, "MP3", 2, 44100)
		);

		CaptureRecorder enc(options, nullptr, audioCapture);
		
		audioCapture->start();	
		enc.start();
		//util::pause();
		enc.stop();
	}
			
	*/

};


bool Tests::stopStreamEncoders = false;


} // namespace av
} // namespace scy


int main(int argc, char** argv)
{
	Logger::instance().add(new ConsoleChannel("debug", LTrace));
	//Logger::instance().setWriter(new AsyncLogWriter);		
	{
#ifdef _MSC_VER
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

		// Create the test application
		Application app;
	
		// Initialize the GarbageCollector in the main thread
		GarbageCollector::instance();

		// Preload our video captures in the main thread
		MediaFactory::instance().loadVideo();	
		{
			Tests run(app);	
		}	

		// Shutdown the media factory and release devices
		MediaFactory::instance().unloadVideo();		
		MediaFactory::shutdown();	
	
		// Wait for user intervention before finalizing
		scy::pause();
			
		// Finalize the application to free all memory
		app.finalize();
	}
	
	// Cleanup singleton instances
	GarbageCollector::shutdown();
	Logger::shutdown();
	return 0;
}




		// Note: 2 tiny mem leaks (964 bytes) are from OpenCV
			
	// Free all pointers pending garbage collection
	//
	// Do this before shutting down the MediaFactory as
	// capture instances may be pending deletion and we 
	// need to dereference the implementation instances
	// so system devices can be properly released.
	//GarbageCollector::instance().finalize();

	//MediaFactory::instance().unloadVideo();
	//MediaFactory::shutdown();
	//MediaFactory::instance().unloadAudio();
	//MediaFactory::uninitialize();