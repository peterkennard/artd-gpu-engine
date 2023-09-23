#pragma once

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class ARTD_API_GPU_ENGINE TimingContext
{
public:
	INL         TimingContext() {}

	INL         ~TimingContext() {}

	/** @brief get the current frame number provided by calling task manager
	 * @return the current frame number
	 */
	INL int     frameNumber() const { return(frameNumber_); }
	/** @brief get the current frame's "simulation" time
	 *
	 * This time is set and fixed for the current task loop frame.
	 * @return double seconds since Unix epoch 1/1/1970:0...
	 * to at least microsecond precision.  This may not be "real time"
	 * depending on speed of a "virtual" simulation.
	 * @return simlation time of current task frame.
	*/
	INL double  frameTime() const { return(frameTime_); }

	/**
	 * @return frameTime() of the prior frameNumber's update.
	 * */
	INL double  lastFrameTime() const { return( lastFrameTime_ ); }

	/** @brief return difference between prior and current frame's simulation time.
	 * @return delta time in seconds.
	 */
    INL double  lastFrameDt() const { return(elapsedSinceLast_); }

    INL bool isDebugFrame() const {
        return(debugFrame_);
    }

protected:

	int     frameNumber_;
	double  frameTime_;
	double  lastFrameTime_;
	double  elapsedSinceLast_;
	bool    debugFrame_ = false;
};

#undef INL

ARTD_END


