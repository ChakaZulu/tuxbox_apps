function update()
{
	parent.headerUpdateEPGData(serviceName, nowT, nowD, nowSt, nextT, nextD, nextSt);
	parent.headerUpdateStatusBar(diskGB, diskH, vpid, apid, ip, lock, upTime);
	parent.headerUpdateVolumeBar(volume, mute);
	parent.headerUpdateChannelStatusBar(dolby, crypt, format);
	parent.headerUpdateRecording(recording);
}
