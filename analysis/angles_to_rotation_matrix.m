% Takes a vector of three angles and returns a rotation matrix
% that can be multiplied by a 3D vector to rotate it by those angles
function M = angles_to_rotation_matrix(angle)
  X = eye(3,3);
  Y = eye(3,3);
  Z = eye(3,3);

  X(2,2) = cosd(angle(1));
  X(2,3) = -sind(angle(1));
  X(3,2) = sind(angle(1));
  X(3,3) = cosd(angle(1));

  Y(1,1) = cosd(angle(2));
  Y(1,3) = sind(angle(2));
  Y(3,1) = -sind(angle(2));
  Y(3,3) = cosd(angle(2));

  Z(1,1) = cosd(angle(3));
  Z(1,2) = -sind(angle(3));
  Z(2,1) = sind(angle(3));
  Z(2,2) = cosd(angle(3));

  M = Z*Y*X;
end
