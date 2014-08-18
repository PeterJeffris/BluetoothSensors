% Analysis to determine accelerometer and gyro seperations
% Written by Peter Jeffris for the University of Colorado 
% Department of Mechanical Engineering

% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.

% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.

% You should have received a copy of the GNU General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>. 
pkg load quaternion
pkg load optim
close all
if ~exist('imu_calibration_state')
clear all
imu_calibration_state = 0

data = dlmread('sensor4_calibration_c.csv',',',1,0);

delta = data(:,1)/1000000;
acc = data(:,2:4);          %acceleration data, [x y z]
gyro = data(:,5:7);         %gyro [x y z]
alt = data(:,8);
temp = data(:,9);
time = cumsum(delta);

% Find the variance around a central measurement
width = 100;
filtered_var = zeros(length(acc),1);
for i = 1+width:length(acc)-width
  forward = sum(var(acc(i:i+width,:)));
  backward = sum(var(acc(i-width:i,:)));
  filtered_var(i) = forward+backward;
end
sorted_filtered_var = sort(filtered_var);

% Find static periods based on the variance of the central measurement
static_period = zeros(length(acc),1);
cutoff = sorted_filtered_var(floor(length(filtered_var)*2/3));
for i = 1+width:length(acc)-width
  if (filtered_var(i) < cutoff)
    static_period(i) = 1;
  end
end    

% Find the intervals
static_interval = [];
i = 1;
while (i < length(static_period))
  if (static_period(i) == 1)
    j = i + 1;
    while (j < length(static_period)) && (static_period(j) == 1)
      j = j + 1;
    end
    static_interval = [static_interval,[j-i; i; j]];
    i = j;
  end
  i = i + 1;
end

% Remove the small intervals due slight movements
min_period = mean(static_interval(1,2:end-1))/3;
usable_period = [];
for i = 1:length(static_interval)
  if (static_interval(1,i) > min_period)
    usable_period = [usable_period,i];
  else
    static_period(static_interval(2,i):static_interval(3,i)) = zeros(static_interval(1,i)+1,1);
  end
end
static_interval = static_interval(2:3,usable_period);

figure
hold on
plot(filtered_var)
plot(static_period)
title('Filtered variance and static periods')
hold off

figure
plot(sorted_filtered_var)
title('Filtered variance distribution')
ylabel('Variance')

figure
hold on
plot(time,acc)
plot(time,static_period*10,'k')
hold off
title('Acceleration')
xlabel('Time (sec)')
ylabel('Acceleration (m/s^2)')
legend('x acceleration','y acceleration','z acceleration')

figure
hold on
plot(time,gyro)
plot(time,static_period*50,'k')
hold off
title('Rotational Rate')
xlabel('Time (sec)')
ylabel('Rotational Rate (deg/s)')
legend('x rate','y rate','z rate')

% Compute the acceleration mean during each static window
static_means = zeros(3,length(static_interval));
static_variances = zeros(3,length(static_interval));
for i = 1:length(static_interval)
  static_means(:,i) = mean(acc(static_interval(1,i):static_interval(2,i),:));
  static_variances(:,i) = var(acc(static_interval(1,i):static_interval(2,i),:));
end

% Looking for unusual static calibration periods
figure
bar(sqrt(sum(static_variances.^2)))
title('Variances of the static period data')
ylabel('Static variance')
xlabel('Period')

% Local gravity calculation from wikipedia
latitude = 40.0176;
altitude = 1655;
gravity = 9.780327 *(1+0.0053024*sin(latitude)^2-0.0000058*sin(2*latitude)^2) - 3.155*10^-7*altitude;

%levenberg-marquadt algorithim setup
init = [1 1 1 0 0 0 0 0 0];
stol = .0001;
niter = 20;
wt1 = ones(1,length(static_means));
dp = .001*ones(size(init));
dFdp = 'dfdp';
function [y] = acc_mag(x,p,a)
  a = [p(1) p(4) p(5); 0 p(2) p(6); 0 0 p(3)]*a;
  a = a + diag([p(7),p(8),p(9)])*ones(size(a));
  y = sqrt(sum(a(:,x).^2));
end
F = @(x,p) acc_mag(x,p,static_means);
[f1, p1, kvg1, iter1, corp1, covp1, covr1, stdresid1, Z1, r21] = leasqr ([1:length(static_means)],ones(1,length(static_means))*gravity,init,F, stol, niter, wt1, dp, dFdp);

accelerometer_correction = [p1(1) p1(4) p1(5); 0 p1(2) p1(6); 0 0 p1(3)];
accelerometer_bias = diag([p1(7) p1(8) p1(9)]);

% Look at the accelerometer biases
figure
plot([time(1), time(end)],[gravity, gravity],'k')
hold on
plot(time,sqrt(sum(acc(:,:)'.^2)),'r')
plot(time,sqrt(sum((accelerometer_correction*acc(:,:)'+accelerometer_bias*ones(size(acc'))).^2)),'b')
hold off
legend('Gravity','Raw Acceleration','Corrected Acceleration')
xlabel('Time (sec)')
ylabel('Acceleration Magnitude')
title('Accelerometer biases')
end

%Find the rotations from the acceleration vectors
gravity_rotations = repmat(quaternion(1),1,length(static_means)-1)
for i = 1:length(gravity_rotations)
  gravity_rotations(i) = quaternion_from_vectors(static_means(:,i+1),static_means(:,i));
end

function [y] = gyro_delta(x,p,l,g,a,dt)
  T = [p(1) p(4) p(5)
       p(7) p(2) p(6)
       p(8) p(9) p(3)];
  % For each rotation estimate the error
  for n = 1:length(x)
    % Apply the correction solution to the gyro data
    w = T*g(:,l(2,n):l(1,n+1));
    dtl = dt(l(2,n):l(1,n+1));

    % Initialize the quaternion with no rotation
    q = quaternion(1);

    % Integrate all gyro data
    for k = 1:length(w)-1
      q = q + .5*(q*quaternion(0,w(1,k),w(2,k),w(3,k)))*dtl(k);
      q = q/norm(q);
    end

    % Find the rotation between static periods via gravity
    v1 = a(:,n)';
    v2 = a(:,n+1)';
    vdot = dot(v1,v2);
    % Handle the case that the vectors are 180 in oposite directions
    if (vdot < -0.999999)
      axis = cross([1 0 0],v1);
      if (norm(axis) < 0.000001)
	axis = cross([0 1 0],v1);
      end
      axis = axis/norm(axis);
      qa = rot2q(axis,pi);
    % Handle all other cases
    else
      axis = cross(v1,v2);
      angle = sqrt(norm(a(n))^2*norm(a(n+1))^2) + vdot;
      qa = quaternion(angle,axis(1),axis(2),axis(3));
      qa = qa/norm(qa);
    end
    y(n) = norm(qa-q);
  end
end
      
init = [1 1 1 0 0 0 0 0 0];
stol = .0001;
niter = 20;
wt1 = ones(1,length(static_means)-1);
dp = .001*ones(size(init));
dFdp = 'dfdp';
F = @(x,p) gyro_delta(x,p,static_interval,gyro',static_means,delta);
[f2, p2, kvg2, iter2, corp2, covp2, covr2, stdresid2, Z2, r22] = leasqr([1:length(static_means)-1],zeros(1,length(static_means)-1),init,F, stol, niter, wt1, dp, dFdp);

