#ifndef DISABLE_FILE

#ifndef __record_h
#define __record_h

#include <libsig_comp.h>
#include <lib/dvb/dvb.h>
#include <lib/base/ebase.h>
#include <lib/base/message.h>
#include <lib/base/thread.h>
#include <lib/base/eerror.h>
#include <lib/base/estring.h>
#include <lib/base/buffer.h>

#include <set>

/**
 * \brief The DVBRecorder
 *
 * A class which can record a TS consisting of given pids. The
 * recorder runs in a seperate thread and can be controlled 
 * asynchronously. Internally this is done with a \c eMessagePump.
 * \todo Howto disable this warning?
 */

class eDVBRecorder: private eThread, public Object
{
	enum { stateRunning = 1, stateStopped = 0 }state;
	struct eDVBRecorderMessage
	{
		enum eCode
		{
			rWriteError // disk full etc.
		} code;
		union
		{
			const char *filename;
			int pid;
			struct
			{
				void *data;
				int len;
			} write;
		};
		eDVBRecorderMessage() { }
		eDVBRecorderMessage(eCode code): code(code) { }
		eDVBRecorderMessage(eCode code, const char *filename): code(code), filename(filename) { }
		eDVBRecorderMessage(eCode code, int pid): code(code), pid(pid) { }
		eDVBRecorderMessage(eCode code, void *data, int len): code(code) { write.data=data; write.len=len; }
	};
	struct pid_t
	{
		int pid;
		int fd;
		bool operator < (const pid_t &p) const
		{
			return pid < p.pid;
		}
	};

	std::set<pid_t> pids, newpids;
	eFixedMessagePump<eDVBRecorderMessage> rmessagepump;

	int splits, splitsize, size, dvrfd, outfd;

//	pthread_mutex_t bufferLock;

	eString filename;

	char buf[65536+188];  // 188 bytes vor manual inject.. in buffer
	int bufptr;

	void thread();
	void gotBackMessage(const eDVBRecorderMessage &msg);
	inline int flushBuffer();
	void openFile(int suffix=0);
public:
	void PMTready(int error);
	eAUTable<PMT> tPMT;
	eString getFilename() { return filename; }
	eServiceReferenceDVB recRef;
	bool scrambled;
		/// the constructor
	eDVBRecorder(PMT *);
		/// the destructor
	~eDVBRecorder();

	/**
	 * \brief Opens a file
	 *
	 * This call will open a new filename. It will close open files.
	 * \param filename Pointer to a filename. Existing files will be deleted. (life is cruel)
	 */
	void open(const char *filename);

	/**
	 * \brief Adds a PID.
	 *
	 * This call will add a PID to record. Recording doesn't start until using \c start().
	 * \sa eDVBRecorder::start
	 */
	std::pair<std::set<eDVBRecorder::pid_t>::iterator,bool> addPID(int pid);

	/**
	 * \brief Removes a PID.
	 *
	 * This call will remove a PID.
	 */
	void removePID(int pid);

	/**
	 * \brief Start recording.
	 *
	 * This call will start the recording. You can stop it with \c stop().
	 * \sa eDVBRecorder::stop
	 */
	void start();

	/**
	 * \brief Stop recording.
	 *
	 * This call will pause the recording. You can restart it (and append data) with \c start again.
	 */
	void stop();

	/**
	 * \brief Closes recording file.
	 *
	 * This will close the recorded file.
	 */
	void close();

	/**
	 * \brief Writes a section into the stream.
	 *
	 * This will generate aligned ts packets and thus write a section to the stream. CRC must be calculated by caller.
	 * File must be already opened.
	 * Len will be fetched out of table.
	 */
	void writeSection(void *data, int pid);

	/**
	 * \brief Adds a PID to running recording.
	 *
	 * This call will add a new PID to a running record. After add all new PIDs .. \c validatePIDs() must be called.
	 * \sa eDVBRecorder::addNewPID
	 */
	void addNewPID(int pid);

	/**
	 * \brief Validate all PIDs.
	 *
	 * This call will remove all PID they never contains in the pmt, and add all new PIDs. All PIDs must set with \c addNewPID before.
	 * \sa eDVBRecorder::validatePIDs
	 */
	void validatePIDs();

	enum { recWriteError };
	Signal1<void,int> recMessage;
};

#endif

#endif //DISABLE_FILE
