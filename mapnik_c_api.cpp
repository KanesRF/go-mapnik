
// Formatted with: astyle  --style=google --pad-oper --add-brackets

#include <mapnik/debug.hpp>
#include <mapnik/version.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/color.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>


#include "mapnik3x_compatibility.hpp"

// vector output api
#include "vector_tile_compression.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_util.hpp"
#include "vector_tile_projection.hpp"

// vector input api
#include "vector_tile_datasource.hpp"
#include "vector_tile.pb.h"

#if MAPNIK_VERSION < 300000
#define MAPNIK_2
#endif

#ifdef MAPNIK_2
#include <mapnik/graphics.hpp>
#endif

#include "mapnik_c_api.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

const int mapnik_version = MAPNIK_VERSION;
const char *mapnik_version_string = MAPNIK_VERSION_STRING;
const int mapnik_version_major = MAPNIK_MAJOR_VERSION;
const int mapnik_version_minor = MAPNIK_MINOR_VERSION;
const int mapnik_version_patch = MAPNIK_PATCH_VERSION;
const int mapnik_version_release = MAPNIK_VERSION_IS_RELEASE;

#ifdef MAPNIK_2
    typedef mapnik::image_32 mapnik_rgba_image;
#else
    typedef mapnik::image_rgba8 mapnik_rgba_image;
#endif


static std::string * register_err;

inline void mapnik_register_reset_last_error() {
    if (register_err) {
        delete register_err;
        register_err = NULL;
    }
}

int mapnik_register_datasources(const char* path) {
    mapnik_register_reset_last_error();
    try {
#if MAPNIK_VERSION >= 200200
        mapnik::datasource_cache::instance().register_datasources(path);
#else
        mapnik::datasource_cache::instance()->register_datasources(path);
#endif
        return 0;
    } catch (std::exception const& ex) {
        register_err = new std::string(ex.what());
        return -1;
    }
}

int mapnik_register_fonts(const char* path) {
    mapnik_register_reset_last_error();
    try {
        mapnik::freetype_engine::register_fonts(path);
        return 0;
    } catch (std::exception const& ex) {
        register_err = new std::string(ex.what());
        return -1;
    }
}

const char *mapnik_register_last_error() {
    if (register_err) {
        return register_err->c_str();
    }
    return NULL;
}

void mapnik_logging_set_severity(int level) {
    mapnik::logger::severity_type severity;
    switch (level) {
    case MAPNIK_DEBUG:
        severity = mapnik::logger::debug;
        break;
    case MAPNIK_WARN:
        severity = mapnik::logger::warn;
        break;
    case MAPNIK_ERROR:
        severity = mapnik::logger::error;
        break;
    default:
        severity = mapnik::logger::none;
        break;
    }
    mapnik::logger::instance().set_severity(severity);
}

struct _mapnik_map_t {
    mapnik::Map * m;
    std::string * err;
};

mapnik_map_t * mapnik_map(unsigned width, unsigned height) {
    mapnik_map_t * map = new mapnik_map_t;
    map->m = new mapnik::Map(width, height);
    map->err = NULL;
    return map;
}

void mapnik_map_free(mapnik_map_t * m) {
    if (m)  {
        if (m->m) {
            delete m->m;
        }
        if (m->err) {
            delete m->err;
        }
        delete m;
    }
}

inline void mapnik_map_reset_last_error(mapnik_map_t *m) {
    if (m && m->err) {
        delete m->err;
        m->err = NULL;
    }
}

const char * mapnik_map_get_srs(mapnik_map_t * m) {
    if (m && m->m) {
        return m->m->srs().c_str();
    }
    return NULL;
}

int mapnik_map_set_srs(mapnik_map_t * m, const char* srs) {
    if (m) {
        m->m->set_srs(srs);
        return 0;
    }
    return -1;
}

double mapnik_map_get_scale_denominator(mapnik_map_t * m) {
    if (m && m->m) {
        return m->m->scale_denominator();
    }
    return 0.0;
}

int mapnik_map_load(mapnik_map_t * m, const char* stylesheet) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik::load_map(*m->m, stylesheet);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

int mapnik_map_zoom_all(mapnik_map_t * m) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            m->m->zoom_all();
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

void mapnik_map_resize(mapnik_map_t *m, unsigned int width, unsigned int height) {
    if (m && m->m) {
        m->m->resize(width, height);
    }
}


MAPNIKCAPICALL void mapnik_map_set_buffer_size(mapnik_map_t * m, int buffer_size) {
    m->m->set_buffer_size(buffer_size);
}

const char *mapnik_map_last_error(mapnik_map_t *m) {
    if (m && m->err) {
        return m->err->c_str();
    }
    return NULL;
}


struct _mapnik_bbox_t {
    mapnik::box2d<double> b;
};

mapnik_bbox_t * mapnik_bbox(double minx, double miny, double maxx, double maxy) {
    mapnik_bbox_t * b = new mapnik_bbox_t;
    b->b = mapnik::box2d<double>(minx, miny, maxx, maxy);
    return b;
}

void mapnik_bbox_free(mapnik_bbox_t * b) {
    if (b) {
        delete b;
    }
}

void mapnik_map_zoom_to_box(mapnik_map_t * m, mapnik_bbox_t * b) {
    if (m && m->m && b) {
        m->m->zoom_to_box(b->b);
    }
}

struct _mapnik_image_t {
    mapnik_rgba_image *i;
    std::string * err;
};

inline void mapnik_image_reset_last_error(mapnik_image_t *i) {
    if (i && i->err) {
        delete i->err;
        i->err = NULL;
    }
}

void mapnik_image_free(mapnik_image_t * i) {
    if (i) {
        if (i->i) {
            delete i->i;
        }
        if (i->err) {
            delete i->err;
        }
        delete i;
    }
}

const char *mapnik_image_last_error(mapnik_image_t *i) {
    if (i && i->err) {
        return i->err->c_str();
    }
    return NULL;
}

mapnik_image_t * mapnik_map_render_to_image(mapnik_map_t * m, double scale, double scale_factor) {
    mapnik_map_reset_last_error(m);
    mapnik_rgba_image * im = new mapnik_rgba_image(m->m->width(), m->m->height());
    if (m && m->m) {
        try {
            mapnik::agg_renderer<mapnik_rgba_image> ren(*m->m, *im, scale_factor);
            if (scale > 0.0) {
                ren.apply(scale);
            } else {
                ren.apply();
            }
        } catch (std::exception const& ex) {
            delete im;
            m->err = new std::string(ex.what());
            return NULL;
        }
    }
    mapnik_image_t * i = new mapnik_image_t;
    i->i = im;
    i->err = NULL;
    return i;
}

int mapnik_map_render_to_file(mapnik_map_t * m, const char* filepath, double scale, double scale_factor, const char *format) {
    mapnik_map_reset_last_error(m);
    if (m && m->m) {
        try {
            mapnik_rgba_image buf(m->m->width(), m->m->height());
            mapnik::agg_renderer<mapnik_rgba_image> ren(*m->m, buf, scale_factor);
            if (scale > 0.0) {
                ren.apply(scale);
            } else {
                ren.apply();
            }
            mapnik::save_to_file(buf, filepath, format);
        } catch (std::exception const& ex) {
            m->err = new std::string(ex.what());
            return -1;
        }
        return 0;
    }
    return -1;
}

void mapnik_image_blob_free(mapnik_image_blob_t * b) {
    if (b) {
        if (b->ptr) {
            delete[] b->ptr;
        }
        delete b;
    }
}

mapnik_image_blob_t * mapnik_image_to_blob(mapnik_image_t * i, const char *format) {
    mapnik_image_reset_last_error(i);
    mapnik_image_blob_t * blob = new mapnik_image_blob_t;
    blob->ptr = NULL;
    blob->len = 0;
    if (i && i->i) {
        try {
            std::string s = save_to_string(*(i->i), format);
            blob->len = s.length();
            blob->ptr = new char[blob->len];
            memcpy(blob->ptr, s.c_str(), blob->len);
        } catch (std::exception const& ex) {
            i->err = new std::string(ex.what());
            delete blob;
            return NULL;
        }
    }
    return blob;
}

const uint8_t * mapnik_image_to_raw(mapnik_image_t * i, size_t * size) {
    if (i && i->i) {
        *size = i->i->width() * i->i->height() * 4;
#ifdef MAPNIK_2
        return i->i->raw_data();
#else
        return (uint8_t *)i->i->data();
#endif

    }
    return NULL;
}

mapnik_image_t * mapnik_image_from_raw(const uint8_t * raw, int width, int height) {
    mapnik_image_t * img = new mapnik_image_t;
    img->i = new mapnik_rgba_image(width, height);
#ifdef MAPNIK_2
    memcpy(img->i->raw_data(), raw, width * height * 4);
#else
    memcpy(img->i->data(), raw, width * height * 4);
#endif
    img->err = NULL;
    return img;
}

int mapnik_map_layer_count(mapnik_map_t * m) {
    if (m && m->m) {
        return m->m->layer_count();
    }
    return 0;
}

const char * mapnik_map_layer_name(mapnik_map_t * m, size_t idx) {
    if (m && m->m) {
#ifdef MAPNIK_2
        mapnik::layer const& layer = m->m->getLayer(idx);
#else
        mapnik::layer const& layer = m->m->get_layer(idx);
#endif
        return layer.name().c_str();
    }
    return NULL;
}

int mapnik_map_layer_is_active(mapnik_map_t * m, size_t idx) {
    if (m && m->m) {
#ifdef MAPNIK_2
        mapnik::layer const& layer = m->m->getLayer(idx);
#else
        mapnik::layer const& layer = m->m->get_layer(idx);
#endif
        return layer.active();
    }
    return 0;
}

void mapnik_map_layer_set_active(mapnik_map_t * m, size_t idx, int active) {
    if (m && m->m) {
#ifdef MAPNIK_2
        mapnik::layer &layer = m->m->getLayer(idx);
#else
        mapnik::layer &layer = m->m->get_layer(idx);
#endif
        layer.set_active(active);
    }
}

int mapnik_map_background(mapnik_map_t * m, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
    if (m && m->m) {
        boost::optional<mapnik::color> const &bg = m->m->background();
        if (bg) {
            mapnik::color c = bg.get();
            *r = c.red();
            *g = c.green();
            *b = c.blue();
            *a = c.alpha();
            return 1;
        }
    }
    return 0;
}

void mapnik_map_set_background(mapnik_map_t * m, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (m && m->m) {
        mapnik::color bg(r, g, b, a);
        m->m->set_background(bg);
    }
}

void mapnik_map_set_maximum_extent(mapnik_map_t * m, double x0, double y0, double x1, double y1) {
    if (m && m->m) {
        mapnik::box2d<double> extent(x0, y0, x1, y1);
        m->m->set_maximum_extent(extent);
    }
}

void mapnik_map_reset_maximum_extent(mapnik_map_t * m) {
    if (m && m->m) {
        m->m->reset_maximum_extent();
    }
}



MAPNIKCAPICALL void mapnik_vt_write(mapnik_map_t * m, const unsigned x, const unsigned y, const unsigned z, const char * fname) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);


    double minx,miny,maxx,maxy;
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    merc.xyz(x, y, z, minx, miny, maxx, maxy);
    mapnik::box2d<double> bbox;
    bbox.init(minx,miny,maxx,maxy);

    std::cerr << x << " " << y << " " << z << std::endl;
    std::cerr << bbox << std::endl;

    m->m->zoom_to_box(bbox);

    mapnik::request m_req(256, 256, bbox);
    renderer_type ren(backend, *(m->m), m_req);
    ren.apply();
    std::string buffer;

    tile.SerializeToString(&buffer);
    std::ofstream out(fname);
    out << buffer;
    out.close();
}


MAPNIKCAPICALL void mapnik_vt_load_ds(mapnik_map_t * m, const unsigned x, const unsigned y, const unsigned z, const char * fname) {
    vector_tile::Tile_Layer vtile;

    std::string buffer;
    std::ifstream in(fname);
    in >> buffer;
    in.close();

    vtile.ParseFromString(buffer);

    MAPNIK_SHARED_PTR<mapnik::vector_tile_impl::tile_datasource> ds = MAPNIK_MAKE_SHARED<
                                    mapnik::vector_tile_impl::tile_datasource>(
                                        vtile, x, y, z, 256);

    double minx,miny,maxx,maxy;
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    merc.xyz(x, y, z, minx, miny, maxx, maxy);
    mapnik::box2d<double> bbox;
    bbox.init(minx,miny,maxx,maxy);

    ds->set_envelope(bbox);
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    BOOST_FOREACH(mapnik::attribute_descriptor const& desc, lay_desc.get_descriptors())
    {
        std::cerr << "name:" << desc.get_name() << std::endl;
    }

    // lyr2.set_datasource(ds);
    // lyr2.add_style("style1");
    // map2.MAPNIK_ADD_LAYER(lyr2);
    // mapnik::load_map(map2,"test/data/style.xml");
    // //std::clog << mapnik::save_map_to_string(map2) << "\n";
    // map2.zoom_to_box(bbox);
    // mapnik::image_32 im(map2.width(),map2.height());
    // mapnik::agg_renderer<mapnik::image_32> ren2(map2,im);
    // ren2.apply();
    // if (!mapnik::util::exists("test/fixtures/expected-1.png")) {
    //     mapnik::save_to_file(im.data(),"test/fixtures/expected-1.png","png32");
    // }
    // unsigned diff = testing::compare_images(im.data(),"test/fixtures/expected-1.png");
    // CHECK(0 == diff);
    // if (diff > 0) {
    //     mapnik::save_to_file(im.data(),"test/fixtures/actual-1.png","png32");
    // }




    // typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    // typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    // typedef vector_tile::Tile tile_type;
    // tile_type tile;
    // backend_type backend(tile,16);


    // double minx,miny,maxx,maxy;
    // mapnik::vector_tile_impl::spherical_mercator merc(256);
    // merc.xyz(x, y, z, minx, miny, maxx, maxy);
    // mapnik::box2d<double> bbox;
    // bbox.init(minx,miny,maxx,maxy);

    // std::cerr << x << " " << y << " " << z << std::endl;
    // std::cerr << bbox << std::endl;

    // m->m->zoom_to_box(bbox);

    // mapnik::request m_req(256, 256, bbox);
    // renderer_type ren(backend, *(m->m), m_req);
    // ren.apply();
    // std::string buffer;

    // tile.SerializeToString(&buffer);
    // std::ofstream out(fname);
    // out << buffer;
    // out.close();
}



#ifdef __cplusplus
}
#endif
