#include <mitsuba/render/mesh.h>

NAMESPACE_BEGIN(mitsuba)

template MTS_EXPORT_CORE auto Mesh::ray_intersect(size_t, const Ray3f &);
template MTS_EXPORT_CORE auto Mesh::ray_intersect(size_t, const Ray3fP &);

Mesh::~Mesh() { }

BoundingBox3f Mesh::bbox() const {
    return m_bbox;
}

BoundingBox3f Mesh::bbox(Index index) const {
    Assert(index <= m_face_count);

    auto idx = (const Index *) face(index);
    Assert(idx[0] < m_vertex_count);
    Assert(idx[1] < m_vertex_count);
    Assert(idx[2] < m_vertex_count);

    Point3f v0 = load<Point3f>((float *) vertex(idx[0]));
    Point3f v1 = load<Point3f>((float *) vertex(idx[1]));
    Point3f v2 = load<Point3f>((float *) vertex(idx[2]));

    return BoundingBox3f(
        min(min(v0, v1), v2),
        max(max(v0, v1), v2)
    );
}


namespace {
    constexpr size_t max_vertices = 10;

    size_t sutherland_hodgman(Point3d *input, size_t in_count, Point3d *output,
                              int axis, double split_pos, bool is_minimum) {
        if (in_count < 3)
            return 0;

        Point3d cur        = input[0];
        double sign        = is_minimum ? 1.0 : -1.0;
        double distance    = sign * (cur[axis] - split_pos);
        bool  cur_is_inside = (distance >= 0);
        size_t out_count    = 0;

        for (size_t i=0; i<in_count; ++i) {
            size_t next_idx = i+1;
            if (next_idx == in_count)
                next_idx = 0;

            Point3d next = input[next_idx];
            distance = sign * (next[axis] - split_pos);
            bool next_is_inside = (distance >= 0);

            if (cur_is_inside && next_is_inside) {
                /* Both this and the next vertex are inside, add to the list */
                Assert(out_count + 1 < max_vertices);
                output[out_count++] = next;
            } else if (cur_is_inside && !next_is_inside) {
                /* Going outside -- add the intersection */
                double t = (split_pos - cur[axis]) / (next[axis] - cur[axis]);
                Assert(out_count + 1 < max_vertices);
                Point3d p = cur + (next - cur) * t;
                p[axis] = split_pos; // Avoid roundoff errors
                output[out_count++] = p;
            } else if (!cur_is_inside && next_is_inside) {
                /* Coming back inside -- add the intersection + next vertex */
                double t = (split_pos - cur[axis]) / (next[axis] - cur[axis]);
                Assert(out_count + 2 < max_vertices);
                Point3d p = cur + (next - cur) * t;
                p[axis] = split_pos; // Avoid roundoff errors
                output[out_count++] = p;
                output[out_count++] = next;
            } else {
                /* Entirely outside - do not add anything */
            }
            cur = next;
            cur_is_inside = next_is_inside;
        }
        return out_count;
    }
}

BoundingBox3f Mesh::bbox(Index index, const BoundingBox3f &clip) const {
    /* Reserve room for some additional vertices */
    Point3d vertices1[max_vertices], vertices2[max_vertices];
    size_t nVertices = 3;

    Assert(index <= m_face_count);

    auto idx = (const Index *) face(index);
    Assert(idx[0] < m_vertex_count);
    Assert(idx[1] < m_vertex_count);
    Assert(idx[2] < m_vertex_count);

    Point3f v0 = load<Point3f>((float *) vertex(idx[0]));
    Point3f v1 = load<Point3f>((float *) vertex(idx[1]));
    Point3f v2 = load<Point3f>((float *) vertex(idx[2]));

    /* The kd-tree code will frequently call this function with
       almost-collapsed bounding boxes. It's extremely important not to
       introduce errors in such cases, otherwise the resulting tree will
       incorrectly remove triangles from the associated nodes. Hence, do
       the following computation in double precision! */

    vertices1[0] = Point3d(v0);
    vertices1[1] = Point3d(v1);
    vertices1[2] = Point3d(v2);

    for (int axis=0; axis<3; ++axis) {
        nVertices = sutherland_hodgman(vertices1, nVertices, vertices2, axis,
                                      (double) clip.min[axis], true);
        nVertices = sutherland_hodgman(vertices2, nVertices, vertices1, axis,
                                      (double) clip.max[axis], false);
    }

    BoundingBox3f result;
    for (size_t i = 0; i < nVertices; ++i) {
        /* Convert back into floats (with conservative custom rounding modes) */
        Array<double, 3, false, RoundingMode::Up>   p1Up  (vertices1[i]);
        Array<double, 3, false, RoundingMode::Down> p1Down(vertices1[i]);
        result.min = min(result.min, Point3f(p1Down));
        result.max = max(result.max, Point3f(p1Up));
    }
    result.clip(clip);

    return result;
}

Shape::Size Mesh::primitive_count() const {
    return face_count();
}

std::string Mesh::to_string() const {
    return tfm::format("%s[\n"
        "  name = \"%s\",\n"
        "  bbox = %s,\n"
        "  vertex_count = %i,\n"
        "  vertex_struct = %s,\n"
        "  face_count = %i,\n"
        "  face_struct = %s,\n"
        "]",
        class_()->name(),
        m_name,
        m_bbox,
        m_vertex_count,
        string::indent(m_vertex_struct->to_string()),
        m_face_count,
        string::indent(m_face_struct->to_string())
    );
}

MTS_IMPLEMENT_CLASS(Mesh, Shape)
NAMESPACE_END(mitsuba)
