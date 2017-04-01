#include <mitsuba/core/vector.h>

NAMESPACE_BEGIN(mitsuba)

template MTS_EXPORT_CORE std::pair<Vector3f,  Vector3f>  coordinate_system(const Vector3f& );
template MTS_EXPORT_CORE std::pair<Vector3fP, Vector3fP> coordinate_system(const Vector3fP&);

NAMESPACE_END(mitsuba)