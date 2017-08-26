#include "AIUITest.h"
#include <string.h>
#include <stdlib.h>
#include "jsoncpp/json/json.h"

bool WriteAudioThread::threadLoop()
{
	char audio[1279];
	int len = mFileHelper->read(audio, 1279);

	if (len > 0)
	{
		Buffer* buffer = Buffer::alloc(len);
		memcpy(buffer->data(), audio, len);

		IAIUIMessage * writeMsg = IAIUIMessage::create(AIUIConstant::CMD_WRITE,
			0, 0,  "data_type=audio,sample_rate=16000", buffer);	

		if (NULL != mAgent)
		{
			mAgent->sendMessage(writeMsg);
		}		
		writeMsg->destroy();
		usleep(10 * 1000);
	} else {
		if (mRepeat)
		{
			mFileHelper->rewindReadFile();
		} else {
			IAIUIMessage * stopWrite = IAIUIMessage::create(AIUIConstant::CMD_STOP_WRITE,
				0, 0, "data_type=audio,sample_rate=16000");

			if (NULL != mAgent)
			{
				mAgent->sendMessage(stopWrite);
			}
			stopWrite->destroy();

			mFileHelper->closeReadFile();
			mRun = false;
		}
	}

	return mRun;
}


void* WriteAudioThread::thread_proc(void * paramptr)
{
	WriteAudioThread * self = (WriteAudioThread *)paramptr;

	while (1) {
		if (! self->threadLoop())
			break;
	}
	return NULL;
}

WriteAudioThread::WriteAudioThread(IAIUIAgent* agent, const string& audioPath, bool repeat):
mAgent(agent), mAudioPath(audioPath), mRepeat(repeat), mRun(true), mFileHelper(NULL)
,thread_created(false)
{
	mFileHelper = new FileUtil::DataFileHelper("");
	mFileHelper->openReadFile(mAudioPath, false);
}

WriteAudioThread::~WriteAudioThread()
{
	
}

void WriteAudioThread::stopRun()
{
    if (thread_created) {
        mRun = false;
        void * retarg;
        pthread_join(thread_id, &retarg);
        thread_created = false;
    }
}

bool WriteAudioThread::run()
{
    if (thread_created == false) {
        int rc = pthread_create(&thread_id, NULL, thread_proc, this);
        if (rc != 0) {
            exit(-1);
        }
        thread_created = true;
        return true;
    }

    return false;
}

void TestListener::onEvent(IAIUIEvent& event)
{
	switch (event.getEventType()) {
	case AIUIConstant::EVENT_STATE:
		{
			switch (event.getArg1()) {
			case AIUIConstant::STATE_IDLE:
				{
					cout << "EVENT_STATE:" << "IDLE" << endl;
				} break;

			case AIUIConstant::STATE_READY:
				{
					cout << "EVENT_STATE:" << "READY" << endl;
				} break;

			case AIUIConstant::STATE_WORKING:
				{
					cout << "EVENT_STATE:" << "WORKING" << endl;
				} break;
			}
		} break;

	case AIUIConstant::EVENT_WAKEUP:
		{
			cout << "EVENT_WAKEUP:" << event.getInfo() << endl;
		} break;

	case AIUIConstant::EVENT_SLEEP:
		{
			cout << "EVENT_SLEEP:arg1=" << event.getArg1() << endl;
		} break;

	case AIUIConstant::EVENT_VAD:
		{
			switch (event.getArg1()) {
			case AIUIConstant::VAD_BOS:
				{
					cout << "EVENT_VAD:" << "BOS" << endl;
				} break;

			case AIUIConstant::VAD_EOS:
				{
					cout << "EVENT_VAD:" << "EOS" << endl;
				} break;

			case AIUIConstant::VAD_VOL:
				{
					//						cout << "EVENT_VAD:" << "VOL" << endl;
				} break;
			}
		} break;

	case AIUIConstant::EVENT_RESULT:
		{
			using namespace VA;
			Json::Value bizParamJson;
			Json::Reader reader;
			
			if (!reader.parse(event.getInfo(), bizParamJson, false)) {
				cout << "parse error!" << endl << event.getInfo() << endl;
				break;
			}
			Json::Value data = (bizParamJson["data"])[0];
			Json::Value params = data["params"];
			Json::Value content = (data["content"])[0];
			string sub = params["sub"].asString();
			cout << "EVENT_RESULT:" << sub << endl;

			if (sub == "nlp")
			{
				Json::Value empty;
				Json::Value contentId = content.get("cnt_id", empty);

				if (contentId.empty())
				{
					cout << "Content Id is empty" << endl;
					break;
				}

				string cnt_id = contentId.asString();
				Buffer* buffer = event.getData()->getBinary(cnt_id.c_str());
				string resultStr;

				if (NULL != buffer)
				{
					resultStr = string((char*)buffer->data());

					cout << resultStr << endl;
				}
			}

		}
		break;

	case AIUIConstant::EVENT_ERROR:
		{
			cout << "EVENT_ERROR:" << event.getArg1() << endl;
		} break;
	}
}

AIUITester::AIUITester() : agent(NULL), writeThread(NULL)
{

}

AIUITester::~AIUITester()
{
	if (agent) {
		agent->destroy();
		agent = NULL;
	}
}

void AIUITester::createAgent()
{
	ISpeechUtility::createSingleInstance("", "",
		"appid=595d96d4");

	string paramStr = FileUtil::readFileAsString(CFG_FILE_PATH);
	agent = IAIUIAgent::createAgent(paramStr.c_str(), &listener);
}

void AIUITester::wakeup()
{
	if (NULL != agent)
	{
		IAIUIMessage * wakeupMsg = IAIUIMessage::create(AIUIConstant::CMD_WAKEUP);
		agent->sendMessage(wakeupMsg);
		wakeupMsg->destroy();
	}
}

void AIUITester::start()
{
	if (NULL != agent)
	{
		IAIUIMessage * startMsg = IAIUIMessage::create (AIUIConstant::CMD_START);
		agent->sendMessage(startMsg);
		startMsg->destroy();
	}
}

void AIUITester::stop()
{
	if (NULL != agent)
	{
		IAIUIMessage *stopMsg = IAIUIMessage::create (AIUIConstant::CMD_STOP);
		agent->sendMessage(stopMsg);
		stopMsg->destroy();
	}
}


void AIUITester::write(bool repeat)
{
	if (agent == NULL)
		return;

	if (writeThread == NULL) {
		writeThread = new WriteAudioThread(agent, TEST_AUDIO_PATH,  repeat);
		writeThread->run();
	}	
}



void AIUITester::stopWriteThread()
{
	if (writeThread) {
		writeThread->stopRun();
		delete writeThread;
		writeThread = NULL;
	}
}

void AIUITester::reset()
{
	if (NULL != agent)
	{
		IAIUIMessage * resetMsg = IAIUIMessage::create(AIUIConstant::CMD_RESET);
		agent->sendMessage(resetMsg);
		resetMsg->destroy();
	}
}
void AIUITester::writeText()
{
	if (NULL != agent)
	{
		string text = "刘德华的歌。";
		// textData内存会在Message在内部处理完后自动release掉
		Buffer* textData = Buffer::alloc(text.length());
		text.copy((char*) textData->data(), text.length());

		IAIUIMessage * writeMsg = IAIUIMessage::create(AIUIConstant::CMD_WRITE,
			0,0, "data_type=text", textData);	

		agent->sendMessage(writeMsg);
		writeMsg->destroy();		
	}
}

void AIUITester::destory()
{
	stopWriteThread();

	if (NULL != agent)
	{
		agent->destroy();
		agent = NULL;
	}

	//		SpeechUtility::getUtility()->destroy();
}

void AIUITester::readCmd()
{
	cout << "input cmd:" << endl;

	string cmd;
	while (true)
	{
		cin >> cmd;

		if (cmd == "c")
		{
			cout << "createAgent" << endl;
			createAgent();
		} else if (cmd == "w") {
			cout << "wakeup" << endl;
			wakeup();
		} else if (cmd == "s") {
			cout << "start" << endl;
			start();
		} else if (cmd == "st") {
			cout << "stop" << endl;
			stop();
		} else if (cmd == "d") {
			cout << "destroy" << endl;
			destory();
		} else if (cmd == "r") {
			cout << "reset" << endl;
			reset();
		} else if (cmd == "e") {
			cout << "exit" << endl;
			break;
		} else if (cmd == "wr") {
			cout << "write" << endl;
			write(false);
		} else if (cmd == "wrr") {
			cout << "write repeatly" << endl;
			write(true);
		} else if (cmd == "swrt") {
			cout << "stopWriteThread" << endl;
			stopWriteThread();
		} else if (cmd == "wrt") {
			cout << "writeText" << endl;
			writeText();
		} else if (cmd == "q") {
			destory();
			break;
		} else {
			cout << "invalid cmd, input again." << endl;
		}
	}
}
void AIUITester::test()
{
	//		AIUISetting::setSaveDataLog(true);
	AIUISetting::setAIUIDir(TEST_ROOT_DIR);
	AIUISetting::initLogger(LOG_DIR);

	readCmd();
}
