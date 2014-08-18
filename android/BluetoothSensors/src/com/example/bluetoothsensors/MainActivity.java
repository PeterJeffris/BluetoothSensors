package com.example.bluetoothsensors;

/*Written by Peter Jeffris for the University of Colorado Mechanical Engineering Department

This is an Android interface to a realtime inertial sensor measurement platform. The connection
is made with a bluetooth interface and the measurements are streamed by a realtime sensor platform
when a request code is received. The first operating mode displays data directly to the user, the
other logs the sensor data directly to an internal file.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	

//Libraries used in thsi application
import android.os.Bundle;
import java.nio.ByteBuffer;
import android.os.Environment;
import java.lang.Integer;
import android.widget.TextView;
import android.app.Activity;
import android.widget.Button;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import java.io.BufferedInputStream;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.SecurityException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Set;
import java.util.UUID;
import java.lang.Runnable;
import android.net.Uri;
import android.os.Handler;

//Set the application to inherit from the activity class
public class MainActivity extends Activity {
	// Main action button for the activity
	Button collection_button;
	// Error messages at the bottom of the screen above the button
	TextView error_msg;
	// Realtime results at the top of the screen
	TextView results;
	// State variables for the activity
	boolean bluetooth_connected;
	boolean collection_enabled;
	// State code for the parser, retains state through sleeps to let the buffer fill
	byte state;
	
	// Add initialization code to the super class initialization function
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// Saved variables from the last time the activity was run
		super.onCreate(null);
		// Setup the view resources
		setContentView(R.layout.activity_main);
		// Initialize the state variables to the default
		bluetooth_connected = false;
		collection_enabled = false;
		logging_enabled = false;
		// Initialize the text view objects with the appropriate resources
		error_msg = (TextView)findViewById(R.id.error_msg);
		results = (TextView)findViewById(R.id.results);
		state = DLE;
	}

	// Callback when the activity is being cleaned up 
	@Override
	public void onDestroy() {
		 // If bluetooth is still connected because the app has been unexpectedly terminated try to make
		 // sure the sensor platform knows to stop streaming and cleanly close the bluetooth connection 
		 if (bluetoothSocket != null && bluetoothSocket.isConnected()) {
			 try {
				 endSensorStream();
			 } catch (Exception e) {
				 error_string = getString(R.string.bluetooth_send_error);
				 interfaceCallbackHandler.post(displayErrorCallback);
			 }
			 try {
				 closeBluetooth();
			 } catch (Exception e) {
				 error_string = getString(R.string.bluetooth_close_connection_error);
				 interfaceCallbackHandler.post(displayErrorCallback);
			 }
		 }
		super.onDestroy();	
	}

	// Create the menu bar
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Define the resource used when pressed
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	// Handle interactions with the menu dropdown
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Callback functions based on the resource selected
		switch (item.getItemId()) {
		case R.id.connection_item:
			manageConnection(item);
			return true;
		case R.id.logging_item:
			manageLogging(item);
			return true;
		default:	
			return super.onOptionsItemSelected(item);	
		}
	}

	// Objects used in bluetooth management
	BluetoothAdapter bluetoothAdapter;
	BluetoothSocket bluetoothSocket; 
	BluetoothDevice bluetoothDevice;
	Set<BluetoothDevice> pairedDevices;	
	Intent enableBluetoothIntent;
	OutputStream bluetoothTx;
	BufferedInputStream bluetoothRx;

	// Try to find a the sensors bluetooth device
	private void getBluetoothDevice() throws SecurityException,IOException {
		// Get all devices that have been paired
		pairedDevices = bluetoothAdapter.getBondedDevices();
		// If paired devices are availible check to see if a sensor bluetooth
		// device can be found, throw exceptions that reclect the nature of 
		// the error encountered
		if (pairedDevices.size() > 0) {
			for(BluetoothDevice device : pairedDevices) {
				if(device.getName().startsWith("INERTIAL_SENSORS")) {
					bluetoothDevice = device;
					return;
				}
			}
			throw new IOException();
		}
		else {
			throw new SecurityException();
		}

	}

	// Make sure the phone has bluetooth hardware
	private void getBluetoothAdapter() throws IOException {
		bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
		if (bluetoothAdapter == null) 
			throw new IOException();
	}

	// Make sure bluetooth is enabled and ask the user to enable if not
	int ENABLE_BLUETOOTH_REQUEST;

	private void enableBluetooth() throws SecurityException {
		if (!bluetoothAdapter.isEnabled()) {
			ENABLE_BLUETOOTH_REQUEST = 1;
			enableBluetoothIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
			startActivityForResult(enableBluetoothIntent, ENABLE_BLUETOOTH_REQUEST);
			if (ENABLE_BLUETOOTH_REQUEST != RESULT_OK) 
				throw new SecurityException();
		}
	}

	// Open a bluetooth socket connection
	private void openBluetooth() throws IOException {
		// The unique identifier is defined by android standards
		//http://developer.android.com/reference/android/bluetooth/BluetoothDevice.html#createRfcommSocketToServiceRecord%28java.util.UUID%29
		UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
		if ((bluetoothSocket == null) || !bluetoothSocket.isConnected()) {
			bluetoothSocket = bluetoothDevice.createRfcommSocketToServiceRecord(uuid);    
			bluetoothSocket.connect();
			bluetoothTx = bluetoothSocket.getOutputStream();
			bluetoothRx = new BufferedInputStream(bluetoothSocket.getInputStream());
		}
	} 

	// Close and clean up a bluetooth socket connection
	private void closeBluetooth() throws IOException {
		if ((bluetoothSocket == null) || !bluetoothSocket.isConnected()) {
			throw new IOException();
		}
		else {
			bluetoothRx.close();
			bluetoothTx.close();
			bluetoothSocket.close();
		}
	}

	// Close and clean up a bluetooth socket connection
	private void clearBluetooth() throws IOException {
		if ((bluetoothSocket == null) || !bluetoothSocket.isConnected()) {
			throw new IOException();
		}
		else {
			bluetoothRx = new BufferedInputStream(bluetoothSocket.getInputStream());
		}
	}

	// Manage a bluetooth connection based on its current state
	// by running the connection functions in the appropriate order
	// and handeling any errors by notifying the user about problems
	public void manageConnection(MenuItem item) {
		error_msg.setText("");
		if (!bluetooth_connected) {
			try {
				getBluetoothAdapter();
			}
			catch (IOException x) {
				error_msg.setText(getString(R.string.bluetooth_capable_error));
			}
			try {
				enableBluetooth();
			}
			catch (SecurityException x) {
				error_msg.setText(getString(R.string.bluetooth_permission_error));
			}
			try {
				getBluetoothDevice();
			}
			catch (IOException x) {
				error_msg.setText(getString(R.string.bluetooth_sensor_pair_error));
			}
			catch (SecurityException x) {
				error_msg.setText(getString(R.string.bluetooth_pair_error));
			}
			try {
				openBluetooth();
			}
			catch (IOException x) {
				error_msg.setText(getString(R.string.bluetooth_open_connection_error));
			}
		}
		else {
			try{
				closeBluetooth();
			}
			catch (IOException x) {
				error_msg.setText(getString(R.string.bluetooth_close_connection_error));
			}
		}
		if (!bluetoothSocket.isConnected()) {
			bluetooth_connected = false; 
			item.setTitle(R.string.connect_string);
		}
		else {
			bluetooth_connected = true;
			item.setTitle(R.string.disconnect_string);
		}
	}

	// Objects for manipulating the logfile
	File log_path;
	File log_file;
	BufferedWriter log_buffer;

	// Open the log file
	public void openLog() throws IOException {
			// Get the download folder location
			log_path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
			// Make folder if the location not already exist
			log_path.mkdirs();
			// Make the new file
			log_file = new File(log_path, "inertial_sensors.csv");
			// Attach the new file to a buffered output so data can be written when it is convenient for the operating system 
			// which makes this app, as well as other running apps faster
			log_buffer = new BufferedWriter(new FileWriter(log_file));
			// Throw any errors up to the caller
	}

		// Close the log file
		public void closeLog() throws IOException{
				// Write the remaining data in the buffer to file
				log_buffer.flush();
				// Close the file cleanly
				log_buffer.close();
				// Let the virtual file system know that a new file has been added for the user
				sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, Uri.fromFile(log_file)));
		}



	private static final int START_SEND = 0xB0;
	private static final int STOP_SEND = 0xB1;
	private static final int SINGLE_SEND = 0xB2;
	
	public void startSensorStream() throws IOException {
			bluetoothTx.write(START_SEND);
	}

	public void endSensorStream() throws IOException {
			bluetoothTx.write(STOP_SEND);
	}

	public void requestSensorData() throws IOException {
			bluetoothTx.write(SINGLE_SEND);
	}

	// Constants for acceleration data conversion processes	
	static final float acc_range = 2;
	static final float acc_bits = 1<<12;
	static final float gravity = (float)9.8067;
	static final float acc_scale = 2*acc_range*gravity/acc_bits;
	static final float gyro_range = 250;
	static final float gyro_bits = 1<<16;
	static final float gyro_scale = 2*gyro_range/gyro_bits;	
	static final int fixed_point_divisor = 1<<4;
	static final float light_bits = 1<<10;
	static final float percent_conversion = 100;
	static final float light_scale = percent_conversion/light_bits;
	
	// Sensor value class for easy access in data structures
	static class SensorValues {
		int delta;
		float[] acceleration = {0, 0, 0};
		float[] rotational_rate = {0, 0, 0};
		float altitude = 0;
		float temperature = 0;
		float light = 0;
		byte errors;
	}

	private SensorValues sensors = new SensorValues();
	
	// Network key constants (also used for error/state codes)
	private static final int DLE = 0x10;
	private static final int STX = 0x20;
	private static final int ETX = 0x30;
	private static final int ACC = 0x01;
	private static final int GYRO = 0x02;
	private static final int BARO = 0x04;
	private static final int PHOTO= 0x08;
	private static final int DELTA= 0x10;
	
	private byte processPacket(int payload_size, byte[] buffer) throws Exception{
		for (int i = 0; i < payload_size; i++) {
			buffer[i] = (byte)bluetoothRx.read(); 
			if (buffer[i] == DLE) {
				buffer[i] = (byte)bluetoothRx.read();
				if (buffer[i] != DLE) 
					return DLE;
				else
					continue;
			}
		}
		if ((byte)bluetoothRx.read() == DLE)
			return (byte)bluetoothRx.read();
		else 
			return DLE;
	}
	
	static final int min_buffer_size = 64;
	static final int buffer_request = 256;
	static final int max_packet_size = 14;
	
	// Collect and convert the acceleration data
	private void getSensorData() throws Exception {
		// The input stream does not merge the bluetooth IO buffer with the buffer
		// accessed with available and read, skipping past this point forces it to
		// happen at some cost, but still much less than complicating the parsing 
		// algorithim with error correction and blocking reads. The following code
		// forces an update of the buffer in the interface if the buffer does not
		// contain enough packets to make the parsing worth while. 
		if (bluetoothRx.available() < min_buffer_size) {
			bluetoothRx.mark(buffer_request+2);
			bluetoothRx.skip(buffer_request+1);
			bluetoothRx.reset();
		}
		
		byte[] payload_buffer = new byte[6];
		while (bluetoothRx.available() > max_packet_size) {
			switch (state) {
			case DLE:
				byte[] packet_header = new byte[2];
				packet_header[0] = (byte)bluetoothRx.read();
				packet_header[1] = (byte)bluetoothRx.read();
				while (bluetoothRx.available() > max_packet_size) {
					if (packet_header[0] == DLE) {
						if (packet_header[1] != DLE) {
							state = packet_header[1];
							break;
						}
						else {
							packet_header[0] = (byte)bluetoothRx.read();
							packet_header[1] = (byte)bluetoothRx.read();
						}
					}
					else {
						packet_header[0] = packet_header[1];
						packet_header[1] = (byte)bluetoothRx.read();
					}
				}
				break;
			case STX:
				state = processPacket(2,payload_buffer);
				if (state == DLE)
					sensors.errors |= DELTA;
				else
					sensors.delta = (payload_buffer[0] << 8) | (payload_buffer[1] & 0xFF);
				break;
			case ACC:
				state = processPacket(6,payload_buffer);
				if (state == DLE)
					sensors.errors |= ACC;
				else {
					sensors.acceleration[0] = ((int)((payload_buffer[1] << 8) | (payload_buffer[0]) & 0xFF))*acc_scale;
					sensors.acceleration[1] = ((int)((payload_buffer[3] << 8) | (payload_buffer[2]) & 0xFF))*acc_scale;
					sensors.acceleration[2] = ((int)((payload_buffer[5] << 8) | (payload_buffer[4]) & 0xFF))*acc_scale;
				}
				break;
			case GYRO:
				state = processPacket(6,payload_buffer);
				if (state == DLE)
					sensors.errors |= GYRO;
				else {
					sensors.rotational_rate[0] = ((int)(payload_buffer[1] << 8) | (payload_buffer[0] & 0xFF))*gyro_scale;
					sensors.rotational_rate[1] = ((int)(payload_buffer[3] << 8) | (payload_buffer[2] & 0xFF))*gyro_scale;
					sensors.rotational_rate[2] = ((int)(payload_buffer[5] << 8) | (payload_buffer[4] & 0xFF))*gyro_scale;
				}
				break;
			case BARO:
				state = processPacket(5,payload_buffer);
				if (state == DLE)
					sensors.errors |= BARO;
				else {
					sensors.altitude = (int)((payload_buffer[1] << 8) | (payload_buffer[0] & 0xFF)) + (float)payload_buffer[2]/fixed_point_divisor;
					sensors.temperature = (int)(payload_buffer[3]) + (float)payload_buffer[4]/fixed_point_divisor;
				}
				break;
			case PHOTO:
				state = processPacket(2,payload_buffer);
				if (state == DLE)
					sensors.errors |= PHOTO;
				else {
					sensors.light = ((int)((payload_buffer[1] << 8) | (payload_buffer[0] & 0xFF)))*light_scale;
				}
				break;
			case ETX:
				if (logging_enabled) 
					writeLog();
				sensors.errors = 0;
				state = DLE;
				break;
			default:
				// Lost byte, go back to looking for the next good packet
				state = DLE;
				break;
			}
		}
	}
	
	// Handler to allow the UI thread and sub-threads to communicate 
	final Handler interfaceCallbackHandler = new Handler();

	// Function called by the display thread to send data to the UI thread for visualization
	final Runnable displayCallback = new Runnable() {
		public void run() {
			results.setText("X Acceleration:" + String.format(" %5.2f ", sensors.acceleration[0]) + " m/s^2\n" +
					"Y Acceleration:" + String.format(" %5.2f ", sensors.acceleration[1]) + " m/s^2\n" +
					"Z Acceleration:" + String.format(" %5.2f ", sensors.acceleration[2]) + " m/s^2\n" +
					"U Angular Velocity:" + String.format("%6.1f ", sensors.rotational_rate[0]) + " deg/s\n" +
					"V Angular Velocity:" + String.format("%6.1f ", sensors.rotational_rate[1]) + " deg/s\n" +
					"W Angular Velocity:" + String.format("%6.1f ", sensors.rotational_rate[2]) + " deg/s\n" +
					"Altitude:" + String.format(" %4.0f", sensors.altitude) + " m\n" +
					"Temperature:" + String.format(" %4.1f", sensors.temperature) + " C\n" +
					"Light:" + String.format(" %2.0f", sensors.light) + "%\n");
		}
	};
	
	// Writes the sensor data to the log
	public void writeLog() throws IOException{
		// Only log if the user has pressed the log button
		if (collection_enabled) {
			// Try to write each value separated by a comma
			try {
				log_buffer.write(Integer.toString(sensors.delta));
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.acceleration[0]).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.acceleration[1]).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.acceleration[2]).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.rotational_rate[0]).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.rotational_rate[1]).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.rotational_rate[2]).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.altitude).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.temperature).toString());
				log_buffer.write(',');
				log_buffer.write(((Float)sensors.light).toString());
				log_buffer.write('\n');
			} catch (IOException e) {
				throw e;
			}
		}
	}

	// Callback function to display the calibration message in the UI 
	final Runnable dispCalibrationCallback = new Runnable() {
		public void run() {
			results.setText(getString(R.string.calibration_string));
		}
	};

	// Callback and global error variable to handle errors from caught exceptions
	String error_string;
	final Runnable displayErrorCallback = new Runnable() {
		public void run() {
			error_msg.setText(error_string);
		}
	};

	// The logging thread activated by the start collection button
	private void startLoggingThread() {
		Thread collectionThread = new Thread() {
			public void run() {
				// Clear out the bluetooth buffer by destroying the old one and replacing it with a new
				try {
					clearBluetooth();
				} catch (IOException e) {
					error_string = getString(R.string.bluetooth_clear_buffer_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}
				// Open the logging file
				try {
					openLog();
				} catch (IOException e) {
					error_string = getString(R.string.log_open_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}

				// Get the date when the record will start and write it to the top of the file
				SimpleDateFormat date_format = new SimpleDateFormat("MM/dd/yyyy-HH:mm:ss");
				String time_stamp = date_format.format(Calendar.getInstance().getTime());
				try {
					log_buffer.write("Collection Started: " + time_stamp + "\n");
				} catch (IOException e) {
					error_string = getString(R.string.log_write_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}
				
				// Tell the sensor platform to start streaming data to the buffer
				try {
					startSensorStream();
				} catch (IOException e) {
					error_string = getString(R.string.bluetooth_send_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}
 
				// Continue collecting until the button is pressed again
				while (collection_enabled) {
					// Parse as many packets as are available
					try {
					    getSensorData();
					}  catch (Exception e) {
						error_string = getString(R.string.bluetooth_receive_error);
						interfaceCallbackHandler.post(displayErrorCallback);
						break;
					}
					// Wait for some time to allow the buffer to fill up again
					try {	
						Thread.sleep(100);
					} catch (InterruptedException e) {
						error_string = getString(R.string.thread_sleep_error);
						interfaceCallbackHandler.post(displayErrorCallback);
						break;
					}
				}
				// The collection has ended so let the sensor platform know to stop streaming
				try {
					endSensorStream();
				} catch (IOException e) {
					error_string = getString(R.string.bluetooth_send_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}

				// Cleanly close the log file
				try {
					closeLog();
				}  catch (IOException e) {
					error_string = getString(R.string.log_close_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}
			}
		};
		collectionThread.start();
	}

	//The logging thread activated by the start collection button
	private void startSamplingThread() {
		Thread collectionThread = new Thread() {
			public void run() {
				// Clear out the bluetooth buffer by destroying the old one and replacing it with a new
				try {
					clearBluetooth();
				} catch (IOException e) {
					error_string = getString(R.string.bluetooth_clear_buffer_error);
					interfaceCallbackHandler.post(displayErrorCallback);
				}
				// Continue collecting until the button is pressed again
				while (collection_enabled) {
					// Tell the sensor platform to send a single sample
					try {
						requestSensorData();
					} catch (IOException e) {
						error_string = getString(R.string.bluetooth_send_error);
						interfaceCallbackHandler.post(displayErrorCallback);
					}
					// Wait for the packets to be buffered
					try {
						Thread.sleep(100);
					} catch (InterruptedException e) {
						error_string = getString(R.string.thread_sleep_error);
						interfaceCallbackHandler.post(displayErrorCallback);
					}
					// Get a data set from the buffer
					try {
						getSensorData();
					}  catch (Exception e) {
						error_string = getString(R.string.bluetooth_receive_error);
						interfaceCallbackHandler.post(displayErrorCallback);
					}
				}
			}
		};
		collectionThread.start();
	}

	// Thread to let the user know to calibrate, then post data back to the UI  
	private void startDisplayThread() {
		Thread displayThread = new Thread() {
			public void run() {
				// Let the user know to leave the sensor platform still
//				if (logging_enabled) {
//					interfaceCallbackHandler.post(dispCalibrationCallback);
//					try {
//						Thread.sleep(2000);
//					} catch (InterruptedException e) {
//						error_string = "Error scheduling thread: " + e.toString();
//						interfaceCallbackHandler.post(displayErrorCallback);
//					}
//				}
				// While the collection button has not been stopped continue to display data every 100 ms 
				while (collection_enabled) {
					// Tell the thread to sleep for a while
					try {
							Thread.sleep(100);
					}    
					catch (InterruptedException e) {
						error_string = "Error scheduling thread: " + e.toString();
						interfaceCallbackHandler.post(displayErrorCallback);
					}
					// Tell the UI to display data
					interfaceCallbackHandler.post(displayCallback);
				}
			}
		};
		displayThread.start();
	}

	public void manageLogging(MenuItem item) {
		if (logging_enabled) {
			item.setTitle(R.string.enable_logging_string);
		}
		else {
			item.setTitle(R.string.disable_logging_string);
		}
		logging_enabled = !logging_enabled;
	}

	boolean logging_enabled;
	// Function called when the collection button is pressed
	public void collect_data(View view) {
		// Make sure there is an active bluetooth connection
		if (bluetooth_connected) {
			// If the button has already been pressed, update it to display the start message for the next press
			if (collection_enabled) {
				collection_enabled = false;
				((Button)view).setText(R.string.enable_transfer_string);
				results.setText("");
			}
			// If the collection was previously not started, update the button message to stop and start display and logging threads
			else {
				collection_enabled = true;
				((Button)view).setText(R.string.disable_transfer_string);
				if (logging_enabled) {
					startLoggingThread();
					startDisplayThread();
				}
				else {
					startSamplingThread();
					startDisplayThread();
				}
			}	
		}
		else
			error_msg.setText(R.string.bluetooth_disconnected_error);
	}
}



