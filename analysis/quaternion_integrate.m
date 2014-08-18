function rotation = quaternion_integrate(initial_quaternion, angular_velocity, delta_time)
  disp('starting integration')
  rotation = initial_quaternion/norm(initial_quaternion);
  for i = 1:length(angular_velocity)
    theta = .5*angular_velocity(1:3,i)*delta_time(i);
    magnitude = norm(theta);
    if (magnitude == 0)
       continue
    end
    axis_vec = theta*sin(magnitude)/magnitude;
    delta_quaternion = quaternion(cos(magnitude),axis_vec(1),axis_vec(2),axis_vec(3));
    rotation = delta_quaternion*rotation;
    rotation = rotation/norm(rotation);
  end
  return
end
