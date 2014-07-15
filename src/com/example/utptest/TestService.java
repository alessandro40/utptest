package com.example.utptest;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class TestService extends Service {
	
	@Override
	public int onStartCommand(Intent i, int flags, int startId)
	{
		Log.d("test", "About to call listen()");
		//myutp.listen();
		myutp.send();
		Log.d("test", "listen() called");
		return Service.START_NOT_STICKY;
	}

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}

}
