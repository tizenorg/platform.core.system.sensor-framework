% SF_Orien
%
% Copyright (c) 2014 Samsung Electronics Co., Ltd.
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
% http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.

% Sensor Fusion Implementation - Orientation Estimation
%
% - Orientation Estimation using Gyroscope as the driving system and Accelerometer+Geo-Magnetic Sensors as Aiding System.
% - Quaternion based approach
% - Estimation and correction of Euler errors and bias errors for gyroscope using Kalman filter


clear

LOW_PASS_FILTERING_ON = 0;

Max_Range_Accel = 39.203407; Min_Range_Accel = -39.204006; Res_Accel = 0.000598;
Max_Range_Gyro = 1146.862549; Min_Range_Gyro = -1146.880005; Res_Gyro = 0.017500;
Max_Range_Magnetic = 1200; Min_Range_Magnetic = -1200; Res_Magnetic = 1;

PI = 3.141593;
GRAVITY = 9.80665;
NON_ZERO_VAL = 0.1;

Bias_Ax = 0.098586;
Bias_Ay = 0.18385;
Bias_Az = 10.084 - GRAVITY;

Bias_Gx = -5.3539;
Bias_Gy = 0.24325;
Bias_Gz = 2.3391;

BUFFER_SIZE = 1065;
MOVING_AVERAGE_WINDOW_LENGTH = 20;
RAD = 57.2957795;
US2S =  1.0 / 1000000.0;

% get accel x,y,z axis data from stored file
Ax = (((dlmread("sensor_data/100ms/roll_pitch_yaw/accel.txt")(:,1))') - Bias_Ax)(1:BUFFER_SIZE);
Ay = (((dlmread("sensor_data/100ms/roll_pitch_yaw/accel.txt")(:,2))') - Bias_Ay)(1:BUFFER_SIZE);
Az = (((dlmread("sensor_data/100ms/roll_pitch_yaw/accel.txt")(:,3))') - Bias_Az)(1:BUFFER_SIZE);
ATime = ((dlmread("sensor_data/100ms/roll_pitch_yaw/accel.txt")(:,4))');

% get gyro x,y,z axis data from stored file
Gx = (((dlmread("sensor_data/100ms/roll_pitch_yaw/gyro.txt")(:,1))') - Bias_Gx)(1:BUFFER_SIZE);
Gy = (((dlmread("sensor_data/100ms/roll_pitch_yaw/gyro.txt")(:,2))') - Bias_Gy)(1:BUFFER_SIZE);
Gz = (((dlmread("sensor_data/100ms/roll_pitch_yaw/gyro.txt")(:,3))') - Bias_Gz)(1:BUFFER_SIZE);
GTime = ((dlmread("sensor_data/100ms/roll_pitch_yaw/gyro.txt")(:,4))');

scale_Gyro = 575;
Gx = Gx/scale_Gyro;
Gy = Gy/scale_Gyro;
Gz = Gz/scale_Gyro;

% get magnetometer x,y,z axis data from stored file
Mx = (((dlmread("sensor_data/100ms/roll_pitch_yaw/magnetic.txt")(:,1))'))(1:BUFFER_SIZE);
My = (((dlmread("sensor_data/100ms/roll_pitch_yaw/magnetic.txt")(:,2))'))(1:BUFFER_SIZE);
Mz = (((dlmread("sensor_data/100ms/roll_pitch_yaw/magnetic.txt")(:,3))'))(1:BUFFER_SIZE);
MTime = ((dlmread("sensor_data/100ms/roll_pitch_yaw/magnetic.txt")(:,4))');

% 1st order smoothening filter
b = [0.9999   0 ];
a = [1.0000000  0.1 ];

% initialize filter output variables to zero
filt_Ax = zeros(1,BUFFER_SIZE);
filt_Ay = zeros(1,BUFFER_SIZE);
filt_Az = zeros(1,BUFFER_SIZE);
filt_Gx = zeros(1,BUFFER_SIZE);
filt_Gy = zeros(1,BUFFER_SIZE);
filt_Gz = zeros(1,BUFFER_SIZE);
filt_Mx = zeros(1,BUFFER_SIZE);
filt_My = zeros(1,BUFFER_SIZE);
filt_Mz = zeros(1,BUFFER_SIZE);

acc_x = zeros(1,BUFFER_SIZE);
acc_y = zeros(1,BUFFER_SIZE);
acc_z = zeros(1,BUFFER_SIZE);
gyr_x = zeros(1,BUFFER_SIZE);
gyr_y = zeros(1,BUFFER_SIZE);
gyr_z = zeros(1,BUFFER_SIZE);
mag_x = zeros(1,BUFFER_SIZE);
mag_y = zeros(1,BUFFER_SIZE);
mag_z = zeros(1,BUFFER_SIZE);

UA = zeros(1,BUFFER_SIZE);
Corrected_Az = zeros(1,BUFFER_SIZE);

% User Acceleration mean and Variance
mean_UA = zeros(1,BUFFER_SIZE);
A_T = zeros(1,BUFFER_SIZE);
G_T = zeros(1,BUFFER_SIZE);
M_T = zeros(1,BUFFER_SIZE);
var_UA = zeros(1,BUFFER_SIZE);
var_Ax = zeros(1,BUFFER_SIZE);
var_Ay = zeros(1,BUFFER_SIZE);
var_Az = zeros(1,BUFFER_SIZE);
var_Gx = zeros(1,BUFFER_SIZE);
var_Gy = zeros(1,BUFFER_SIZE);
var_Gz = zeros(1,BUFFER_SIZE);
var_Mx = zeros(1,BUFFER_SIZE);
var_My = zeros(1,BUFFER_SIZE);
var_Mz = zeros(1,BUFFER_SIZE);

mean_Roll = zeros(1,BUFFER_SIZE);
mean_Pitch = zeros(1,BUFFER_SIZE);
mean_Yaw = zeros(1,BUFFER_SIZE);
var_Roll = zeros(1,BUFFER_SIZE);
var_Pitch = zeros(1,BUFFER_SIZE);
var_Yaw = zeros(1,BUFFER_SIZE);
quat_driv = zeros(BUFFER_SIZE,4);
quat_aid = zeros(BUFFER_SIZE,4);
quat_error = zeros(BUFFER_SIZE,4);
euler = zeros(BUFFER_SIZE,3);
Ro = zeros(3, 3, BUFFER_SIZE);

% Gyroscope Bias Variables
Bx = 0; By = 0; Bz = 0;

% system covariance matrix
Q=[ 1.1 0  0   0 0 0;
	0  1.1 0   0 0 0;
	0  0   1.1 0 0 0;
	0  0   0   0 0 0;
	0  0   0   0 0 0;
	0  0   0   0 0 0;];

% measurement covariance matrix
R=[ 1.01 0    0    0    0    0;
	0    1.01 0    0    0    0;
	0    0    1.01 0    0    0;
	0    0    0    1.01 0    0;
	0    0    0    0    1.01 0;
	0    0    0    0    0    1.01;];

%R=[ 1.01 0    0;
%	0    1.01 0;
%	0    0    1.01;];
	
A_T(1) = 100000;
G_T(1) = 100000;
M_T(1) = 100000;
	
acc_e = [0.0;0.0;1.0]; % gravity vector in earth frame
mag_e = [0.0;1.0;0.0]; % magnetic field vector in eart frame	
	
H = [eye(3) zeros(3,3); zeros(3,6)];
x = zeros(6,BUFFER_SIZE);
e = zeros(1,6);
P = 1 * eye(6);% state covariance matrix

% first order filtering
for i = 1:BUFFER_SIZE
	if LOW_PASS_FILTERING_ON == 1
		if i < 2
			filt_Ax(i) = Ax(i);
			filt_Ay(i) = Ay(i);
			filt_Az(i) = Az(i);
			filt_Gx(i) = Gx(i);
			filt_Gy(i) = Gy(i);
			filt_Gz(i) = Gz(i);
			filt_Mx(i) = Mx(i);
			filt_My(i) = My(i);
			filt_Mz(i) = Mz(i);
		elseif i >= 2
			filt_Ax(i) = -a(2)*filt_Ax(i-1)+b(1)*Ax(i);
			filt_Ay(i) = -a(2)*filt_Ay(i-1)+b(1)*Ay(i);
			filt_Az(i) = -a(2)*filt_Az(i-1)+b(1)*Az(i);
			filt_Gx(i) = -a(2)*filt_Gx(i-1)+b(1)*Gx(i);
			filt_Gy(i) = -a(2)*filt_Gy(i-1)+b(1)*Gy(i);
			filt_Gz(i) = -a(2)*filt_Gz(i-1)+b(1)*Gz(i);
			filt_Mx(i) = -a(2)*filt_Mx(i-1)+b(1)*Mx(i);
			filt_My(i) = -a(2)*filt_My(i-1)+b(1)*My(i);
			filt_Mz(i) = -a(2)*filt_Mz(i-1)+b(1)*Mz(i);
		end
	else
		filt_Ax(i) = Ax(i);
		filt_Ay(i) = Ay(i);
		filt_Az(i) = Az(i);
		filt_Gx(i) = Gx(i);
		filt_Gy(i) = Gy(i);
		filt_Gz(i) = Gz(i);
		filt_Mx(i) = Mx(i);
		filt_My(i) = My(i);
		filt_Mz(i) = Mz(i);
	end
	
	% normalize accelerometer measurements
	norm_acc = 1/sqrt(filt_Ax(i)^2 + filt_Ay(i)^2 + filt_Az(i)^2);
	acc_x(i) = norm_acc * filt_Ax(i);
	acc_y(i) = norm_acc * filt_Ay(i);
	acc_z(i) = norm_acc * filt_Az(i);

	% normalize magnetometer measurements
	norm_mag = 1/sqrt(filt_Mx(i)^2 + filt_My(i)^2 + filt_Mz(i)^2);
	mag_x(i) = norm_mag * filt_Mx(i);
	mag_y(i) = norm_mag * filt_My(i);
	mag_z(i) = norm_mag * filt_Mz(i);
	
	gyr_x(i) = filt_Gx(i) * PI;
	gyr_y(i) = filt_Gy(i) * PI;
	gyr_z(i) = filt_Gz(i) * PI;
	
	UA(i) = sqrt(acc_x(i)^2 + acc_y(i)^2 + acc_z(i)^2) - GRAVITY;
	
	if i <= MOVING_AVERAGE_WINDOW_LENGTH
		mean_UA(i) = UA(i);
		var_UA(i) = NON_ZERO_VAL;
		var_Ax(i) = NON_ZERO_VAL;
		var_Ay(i) = NON_ZERO_VAL;
		var_Az(i) = NON_ZERO_VAL;
		var_Gx(i) = NON_ZERO_VAL;
		var_Gy(i) = NON_ZERO_VAL;
		var_Gz(i) = NON_ZERO_VAL;
		var_Mx(i) = NON_ZERO_VAL;
		var_My(i) = NON_ZERO_VAL;
		var_Mz(i) = NON_ZERO_VAL;
	else
		mean_UA(i) = mean(UA((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_UA(i) = var(UA((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Ax(i) = var(acc_x((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Ay(i) = var(acc_y((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Az(i) = var(acc_z((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Gx(i) = var(gyr_x((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Gy(i) = var(gyr_y((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Gz(i) = var(gyr_z((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Mx(i) = var(mag_x((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_My(i) = var(mag_y((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
		var_Mz(i) = var(mag_z((i-MOVING_AVERAGE_WINDOW_LENGTH):i));
	end
	if i > 1
		A_T(i) = ATime(i) - ATime(i-1);
		G_T(i) = GTime(i) - GTime(i-1);
		M_T(i) = MTime(i) - MTime(i-1);
	end
	
	dt = G_T(i) * US2S;
	
	gyr_x(i) = gyr_x(i) + Bx;
	gyr_y(i) = gyr_y(i) + By;
	gyr_z(i) = gyr_z(i) + Bz;
	
	% Aiding System (Accelerometer + Geomagnetic) quaternion generation
	% gravity vector in body frame
	acc_b =[acc_x(i);acc_y(i);acc_z(i)]; 
	% magnetic field vector in body frame
	mag_b =[mag_x(i);mag_y(i);mag_z(i)]; 

	% compute measurement quaternion with TRIAD algorithm
	acc_b_x_mag_b = cross(acc_b,mag_b);
	acc_e_x_mag_e = cross(acc_e,mag_e);
	M_b = [acc_b acc_b_x_mag_b cross(acc_b_x_mag_b,acc_b)];
	M_e = [acc_e acc_e_x_mag_e cross(acc_e_x_mag_e,acc_e)];
	Rot_m = M_e * M_b'; 	
	quat_aid(i,:) = RotMat2Quat(Rot_m);
	
	% initialization for q
	if i == 1
		q = quat_aid(i,:);
	end
	
	q_t = [q(2) q(3) q(4)]';
	Rtan = (q(1)^2 - q_t'*q_t)*eye(3) + 2*q_t*q_t' - 2*q(1)*[0 -q(4) q(3);q(4) 0 -q(2);-q(3) q(2) 0];
	%F = [[0 gyr_z(i) -gyr_y(i);-gyr_z(i) 0 gyr_x(i);gyr_y(i) -gyr_x(i) 0] Rtan;  eye(3) ((-1/dt) * eye(3))];
	F = [[0 gyr_z(i) -gyr_y(i);-gyr_z(i) 0 gyr_x(i);gyr_y(i) -gyr_x(i) 0] Rtan;  zeros(3,6)];
	%F = [[0 gyr_z(i) -gyr_y(i);-gyr_z(i) 0 gyr_x(i);gyr_y(i) -gyr_x(i) 0] Rtan;  zeros(3,3) ((1/dt) * eye(3))];
	%F = eye(6); 

	% Time Update
	if i > 1
		x(:,i) = F * x(:,i-1);
	end

	% compute covariance of prediction
	P = (F * P * F') + Q;
	
	% Driving System (Gyroscope) quaternion generation
	% convert scaled gyro data to rad/s
	qDot = 0.5 * QuatProd(q, [0 gyr_x(i) gyr_y(i) gyr_z(i)]);

    % Integrate to yield quaternion
    q = q + qDot * dt * PI;
	
	% normalise quaternion
    quat_driv(i,:) = q / norm(q);

	% Kalman Filtering
	quat_error(i,:) = QuatProd(quat_aid(i,:), quat_driv(i,:));
	
	euler_e = Quat2Euler(quat_error(i,:));
	x1 = euler_e(1)'/PI;
	x2 = euler_e(2)'/PI;
	x3 = euler_e(3)'/PI;
	
	q = QuatProd(quat_driv(i,:), [1 x1 x2 x3]) * PI;
	q = q / norm(q);
	
	By = x(4,i);
	Bx = x(5,i);
	Bz = x(6,i);

	if i > 1
		e = [x1 x2 x3 x(4,i) x(5,i) x(6,i)];
	end
	
	for j =1:6
		% compute Kalman gain
		K(:,j) = P(j ,:)./(P(j,j)+R(j,j));
		% update state vector
		x(:,i) = x(:,i) + K(:,j) * e(j);
		% update covariance matrix
		P = ((eye(6) - (K(:,j) * H(j,:))) * P * (eye(6) - (K(:,j) * H(j,:)))') + (K(:,j) * R(j,j) * K(:,j)');
	end
end

euler = Quat2Euler(quat_aid);
roll = euler(:,1)' * RAD;
pitch = euler(:,2)' * RAD;
yaw = euler(:,3)' * RAD;

euler = Quat2Euler(quat_driv);
phi = euler(:,1)' * RAD;
theta = euler(:,2)' * RAD;
psi = -euler(:,3)' * RAD;

euler = Quat2Euler(quat_error);
roll_e = euler(:,1)' * RAD;
pitch_e = euler(:,2)' * RAD;
yaw_e = euler(:,3)' * RAD;

% Plot results
close all

% Rotation Plot Results
hfig=(figure);
scrsz = get(0,'ScreenSize');
set(hfig,'position',scrsz);
subplot(3,1,1)
UA = pitch;
p1 = plot(1:length(UA),UA(1,1:length(UA)),'r');
hold on;
grid on;
UA = theta;
p2 = plot(1:length(UA),UA(1,1:length(UA)),'b');
hold on;
grid on;
UA = pitch_e;
p3 = plot(1:length(UA),UA(1,1:length(UA)),'g');
title(['Pitch']);
legend([p1 p2 p3],'Aiding System', 'Driving System', 'Quaternion based error');
subplot(3,1,2)
UA = roll;
p1 = plot(1:length(UA),UA(1,1:length(UA)),'r');
hold on;
grid on;
UA = phi;
p2 = plot(1:length(UA),UA(1,1:length(UA)),'b');
hold on;
grid on;
UA = roll_e;
p3 = plot(1:length(UA),UA(1,1:length(UA)),'g');
title(['Roll']);
legend([p1 p2 p3],'Aiding System', 'Driving System', 'Quaternion based error');
subplot(3,1,3)
UA = yaw;
p1 = plot(1:length(UA),UA(1,1:length(UA)),'r');
hold on;
grid on;
UA = psi;
p2 = plot(1:length(UA),UA(1,1:length(UA)),'b');
hold on;
grid on;
UA = yaw_e;
p3 = plot(1:length(UA),UA(1,1:length(UA)),'g');
title(['Yaw']);
legend([p1 p2 p3],'Aiding System', 'Driving System', 'Quaternion based error');

% Accelerometer/Gyroscope/Magnetometer scaled Plot results
hfig=(figure);
scrsz = get(0,'ScreenSize');
set(hfig,'position',scrsz);
subplot(3,1,1)
p1 = plot(1:BUFFER_SIZE,acc_x(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,gyr_x(1,1:BUFFER_SIZE),'b');
hold on;
grid on;
p3 = plot(1:BUFFER_SIZE,mag_x(1,1:BUFFER_SIZE),'k');
title(['Accelerometer/Gyroscope/Magnetometer X-Axis Plot']);
legend([p1 p2 p3],'Acc_X', 'Gyr_X', 'Mag_X');
subplot(3,1,2)
p1 = plot(1:BUFFER_SIZE,acc_y(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,gyr_y(1,1:BUFFER_SIZE),'b');
hold on;
grid on;
p3 = plot(1:BUFFER_SIZE,mag_y(1,1:BUFFER_SIZE),'k');
title(['Accelerometer/Gyroscope/Magnetometer Y-Axis Plot']);
legend([p1 p2 p3],'Acc_Y', 'Gyr_Y', 'Mag_Y');
subplot(3,1,3)
p1 = plot(1:BUFFER_SIZE,acc_z(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,gyr_z(1,1:BUFFER_SIZE),'b');
hold on;
grid on;
p3 = plot(1:BUFFER_SIZE,mag_z(1,1:BUFFER_SIZE),'k');
title(['Accelerometer/Gyroscope/Magnetometer Z-Axis Plot']);
legend([p1 p2 p3],'Acc_Z', 'Gyr_Z', 'Mag_Z');

% Accelerometer Raw (vs) filtered output
hfig=(figure);
scrsz = get(0,'ScreenSize');
set(hfig,'position',scrsz);
subplot(3,1,1)
p1 = plot(1:BUFFER_SIZE,Ax(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Ax(1,1:BUFFER_SIZE),'b');
title(['Accelerometer X-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
subplot(3,1,2)
p1 = plot(1:BUFFER_SIZE,Ay(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Ay(1,1:BUFFER_SIZE),'b');
title(['Accelerometer Y-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
subplot(3,1,3)
p1 = plot(1:BUFFER_SIZE,Az(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Az(1,1:BUFFER_SIZE),'b');
title(['Accelerometer Z-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');

% Gyroscope Raw (vs) filtered output
hfig=(figure);
scrsz = get(0,'ScreenSize');
set(hfig,'position',scrsz);
subplot(3,1,1)
p1 = plot(1:BUFFER_SIZE,Gx(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Gx(1,1:BUFFER_SIZE),'b');
title(['Gyroscope X-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
subplot(3,1,2)
p1 = plot(1:BUFFER_SIZE,Gy(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Gy(1,1:BUFFER_SIZE),'b');
title(['Gyroscope Y-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
subplot(3,1,3)
p1 = plot(1:BUFFER_SIZE,Gz(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Gz(1,1:BUFFER_SIZE),'b');
title(['Gyroscope Z-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');

% Magnetometer Raw (vs) filtered output
hfig=(figure);
scrsz = get(0,'ScreenSize');
set(hfig,'position',scrsz);
subplot(3,1,1)
p1 = plot(1:BUFFER_SIZE,Mx(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Mx(1,1:BUFFER_SIZE),'b');
title(['Magnetometer X-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
subplot(3,1,2)
p1 = plot(1:BUFFER_SIZE,My(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_My(1,1:BUFFER_SIZE),'b');
title(['Magnetometer Y-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
subplot(3,1,3)
p1 = plot(1:BUFFER_SIZE,Mz(1,1:BUFFER_SIZE),'r');
hold on;
grid on;
p2 = plot(1:BUFFER_SIZE,filt_Mz(1,1:BUFFER_SIZE),'b');
title(['Magnetometer Z-Axis Plot']);
legend([p1 p2],'input signal','low-pass filtered signal');
