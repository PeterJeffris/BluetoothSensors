function q = euler2quaternion(angles)
  c = cos(angles/2);
  s = sin(angles/2);
  r(1) = c(1)*c(2)*c(3)+s(1)*s(2)*s(3);
  r(2) = s(1)*c(2)*c(3)-c(1)*s(2)*s(3);
  r(3) = c(1)*s(2)*c(3)+s(1)*c(2)*s(3);
  r(4) = c(1)*c(2)*s(3)-s(1)*s(2)*c(3);
  q = quaternion(r(1),r(2),r(3),r(4));
end
