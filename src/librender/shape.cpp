#include <mitsuba/render/kdtree.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/sensor.h>
#include <mitsuba/render/medium.h>
#include <mitsuba/core/plugin.h>

NAMESPACE_BEGIN(mitsuba)

MTS_VARIANT Shape<Float, Spectrum>::Shape(const Properties &props) : m_id(props.id()) {
    for (auto &kv : props.objects()) {
        Emitter *emitter = dynamic_cast<Emitter *>(kv.second.get());
        BSDF *bsdf = dynamic_cast<BSDF *>(kv.second.get());

        if (emitter) {
            if (m_emitter)
                Throw("Only a single Emitter child object can be specified per shape.");
            m_emitter = emitter;
        } else if (bsdf) {
            if (m_bsdf)
                Throw("Only a single BSDF child object can be specified per shape.");
            m_bsdf = bsdf;
        } else {
            Throw("Tried to add an unsupported object of type \"%s\"", kv.second);
        }
    }

    // Create a default diffuse BSDF if needed.
    if (!m_bsdf)
        m_bsdf = PluginManager::instance()->create_object<BSDF>(Properties("diffuse"));
}

MTS_VARIANT Shape<Float, Spectrum>::~Shape() {}

MTS_VARIANT std::string Shape<Float, Spectrum>::id() const {
    return m_id;
}

MTS_VARIANT typename Shape<Float, Spectrum>::PositionSample3f
Shape<Float, Spectrum>::sample_position(Float /*time*/, const Point2f & /*sample*/,
                                        Mask /*active*/) const {
    NotImplementedError("sample_position");
}

MTS_VARIANT Float Shape<Float, Spectrum>::pdf_position(const PositionSample3f & /*ps*/, Mask /*active*/) const {
    NotImplementedError("pdf_position");
}

#if defined(MTS_USE_EMBREE)
MTS_VARIANT RTCGeometry Shape<Float, Spectrum>::embree_geometry(RTCDevice) const {
    NotImplementedError("embree_geometry");
}
#endif

#if defined(MTS_USE_OPTIX)
MTS_VARIANT RTgeometrytriangles Shape<Float, Spectrum>::optix_geometry(RTcontext) {
    NotImplementedError("optix_geometry");
}
#endif

MTS_VARIANT typename Shape<Float, Spectrum>::DirectionSample3f
Shape<Float, Spectrum>::sample_direction(const Interaction3f &it,
                                         const Point2f &sample,
                                         Mask active) const {
    DirectionSample3f ds(sample_position(it.time, sample, active));
    ds.d = ds.p - it.p;

    Float dist_squared = squared_norm(ds.d);
    ds.dist = sqrt(dist_squared);
    ds.d /= ds.dist;

    Float dp = abs_dot(ds.d, ds.n);
    ds.pdf *= select(neq(dp, 0.f), dist_squared / dp, 0.f);
    ds.object = (const Object *) this;

    return ds;
}

MTS_VARIANT Float Shape<Float, Spectrum>::pdf_direction(const Interaction3f & /*it*/,
                                                        const DirectionSample3f &ds,
                                                        Mask active) const {
    Float pdf = pdf_position(ds, active),
           dp = abs_dot(ds.d, ds.n);

    pdf *= select(neq(dp, 0.f), (ds.dist * ds.dist) / dp, 0.f);

    return pdf;
}

MTS_VARIANT std::pair<typename Shape<Float, Spectrum>::Mask, Float>
Shape<Float, Spectrum>::ray_intersect(const Ray3f & /*ray*/, Float * /*cache*/,
                                      Mask /*active*/) const {
    NotImplementedError("ray_intersect");
}

MTS_VARIANT typename Shape<Float, Spectrum>::Mask Shape<Float, Spectrum>::ray_test(const Ray3f &ray,
                                                                                   Mask /*active*/) const {
    Float unused[MTS_KD_INTERSECTION_CACHE_SIZE];
    return ray_intersect(ray, unused).first;
}

MTS_VARIANT void Shape<Float, Spectrum>::fill_surface_interaction(const Ray3f & /*ray*/,
                                                                  const Float * /*cache*/,
                                                                  SurfaceInteraction3f & /*si*/,
                                                                  Mask /*active*/) const {
    NotImplementedError("fill_surface_interaction");
}

MTS_VARIANT typename Shape<Float, Spectrum>::SurfaceInteraction3f
Shape<Float, Spectrum>::ray_intersect(const Ray3f &ray, Mask active) const {
    SurfaceInteraction3f si = zero<SurfaceInteraction3f>();
    Float cache[MTS_KD_INTERSECTION_CACHE_SIZE];
    Mask success = false;
    std::tie(success, si.t) = ray_intersect(ray, cache, active);
    active &= success;
    if (any(active))
        fill_surface_interaction(ray, cache, si, active);
    return si;
}

MTS_VARIANT std::pair<typename Shape<Float, Spectrum>::Vector3f, typename Shape<Float, Spectrum>::Vector3f>
Shape<Float, Spectrum>::normal_derivative(const SurfaceInteraction3f & /*si*/,
                                          bool /*shading_frame*/, Mask /*active*/) const {
    NotImplementedError("normal_derivative");
}

MTS_VARIANT typename Shape<Float, Spectrum>::ScalarFloat
Shape<Float, Spectrum>::surface_area() const {
    NotImplementedError("surface_area");
}

MTS_VARIANT typename Shape<Float, Spectrum>::ScalarBoundingBox3f
Shape<Float, Spectrum>::bbox(Index) const {
    return bbox();
}

MTS_VARIANT typename Shape<Float, Spectrum>::ScalarBoundingBox3f
Shape<Float, Spectrum>::bbox(Index index, const ScalarBoundingBox3f &clip) const {
    ScalarBoundingBox3f result = bbox(index);
    result.clip(clip);
    return result;
}

MTS_VARIANT typename Shape<Float, Spectrum>::Size
Shape<Float, Spectrum>::primitive_count() const {
    return 1;
}

MTS_VARIANT typename Shape<Float, Spectrum>::Size
Shape<Float, Spectrum>::effective_primitive_count() const {
    return primitive_count();
}

MTS_VARIANT std::vector<ref<Object>>
Shape<Float, Spectrum>::children() {
    std::vector<ref<Object>> result;

    if (m_bsdf)
        result.push_back(m_bsdf.get());
    if (m_emitter)
        result.push_back(m_emitter.get());
    if (m_sensor)
        result.push_back(m_sensor.get());
    if (m_interior_medium)
        result.push_back(m_interior_medium.get());
    if (m_exterior_medium)
        result.push_back(m_exterior_medium.get());

    return result;
}

MTS_INSTANTIATE_OBJECT(Shape)
NAMESPACE_END(mitsuba)
