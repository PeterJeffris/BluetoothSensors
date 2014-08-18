function rotation = quaternion_from_vectors(v1,v2)
  angle = acos(dot(v1/norm(v1),v1/norm(v2)));
  axis = cross(v1,v2);
  axis = axis/norm(axis)*sin(.5*angle);
  rotation = quaternion(cos(.5*angle),axis(1),axis(2),axis(3));
  return 
end
