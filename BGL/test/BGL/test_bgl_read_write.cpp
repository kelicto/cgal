#include <CGAL/Simple_cartesian.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Surface_mesh.h>

#include <CGAL/Polyhedron_items_with_id_3.h>
#include <CGAL/Polyhedron_3.h>

#include <CGAL/Linear_cell_complex_for_bgl_combinatorial_map_helper.h>
#include <CGAL/boost/graph/graph_traits_Linear_cell_complex_for_combinatorial_map.h>

#if defined(CGAL_USE_OPENMESH)
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <CGAL/boost/graph/graph_traits_PolyMesh_ArrayKernelT.h>
#endif

#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/io.h>
#include <CGAL/Origin.h>

#include <iostream>
#include <fstream>


typedef CGAL::Simple_cartesian<double>                                                   Kernel;
typedef Kernel::Point_2                                                                  Point_2;
typedef Kernel::Point_3                                                                  Point;
typedef Kernel::Vector_3                                                                 Vector;

typedef CGAL::Exact_predicates_inexact_constructions_kernel                              EPICK;

typedef CGAL::Polyhedron_3<Kernel, CGAL::Polyhedron_items_with_id_3>                     Polyhedron;

typedef CGAL::Surface_mesh<Point>                                                        SM;

typedef CGAL::Linear_cell_complex_traits<3, Kernel>                                      MyTraits;
typedef CGAL::Linear_cell_complex_for_bgl_combinatorial_map_helper<2, 3, MyTraits>::type LCC;

#if defined(CGAL_USE_OPENMESH)

typedef OpenMesh::PolyMesh_ArrayKernelT</* MyTraits*/>                                   OMesh;

#endif

const double epsilon=1e-6;
template < typename Type >
bool approx_equal_nt(const Type &t1, const Type &t2)
{
      if (t1 == t2)
              return true;
      if (CGAL::abs(t1 - t2) / (CGAL::max)(CGAL::abs(t1), CGAL::abs(t2)) < std::abs(t1)*epsilon)
              return true;
      std::cout << " Approximate comparison failed between : " << t1 << "  and  " << t2 << "\n";
      return false;
}


template <typename Mesh, typename VPM1, typename VPM2, typename VIM1, typename VIM2>
bool are_equal_meshes(const Mesh& fg1, const VPM1 vpm1, const Mesh& fg2, const VPM2 vpm2,
                      const VIM1 vim1, const VIM2 vim2)
{
  typedef typename boost::property_traits<VPM1>::value_type P;

  if(num_vertices(fg1) != num_vertices(fg2) ||
     num_halfedges(fg1) != num_halfedges(fg2) ||
     num_edges(fg1) != num_edges(fg2) ||
     num_faces(fg1) != num_faces(fg2))
    return false;
  std::vector<P> fg1_points, fg2_points;
  for(auto v : vertices(fg1))
  {
    fg1_points.push_back(get(vpm1, v));
  }

  for(auto v : vertices(fg2))
    fg2_points.push_back(get(vpm2, v));

  for(std::size_t id = 0; id < fg1_points.size(); ++id)
  {
    P p1 = fg1_points[id];
    P p2 = fg2_points[id];
    if (! (approx_equal_nt(p1.x(), p2.x())
           && approx_equal_nt(p1.y(), p2.y())
           && approx_equal_nt(p1.z(), p2.z())))
      return false;
  }
  fg1_points.clear();
  fg1_points.shrink_to_fit();
  fg2_points.clear();
  fg2_points.shrink_to_fit();

  std::vector<std::size_t> fg1_faces;
  std::vector<std::size_t> fg2_faces;
  for(auto f : faces(fg1))
    for(auto v : CGAL::vertices_around_face(halfedge(f, fg1), fg1))
      fg1_faces.push_back(get(vim1, v));

  for(auto f : faces(fg2))
    for(auto v : CGAL::vertices_around_face(halfedge(f, fg2), fg2))
      fg2_faces.push_back(get(vim2, v));

  if(fg1_points != fg2_points)
    return false;

  return true;
}

template <typename Mesh>
bool are_equal_meshes(const Mesh& fg1, const Mesh& fg2)
{
  typedef typename CGAL::GetInitializedVertexIndexMap<Mesh>::const_type      VIM;
  VIM vim1 = CGAL::get_initialized_vertex_index_map(fg1);
  VIM vim2 = CGAL::get_initialized_vertex_index_map(fg2);
  return are_equal_meshes(fg1, get(CGAL::vertex_point, fg1), fg2, get(CGAL::vertex_point, fg2), vim1, vim2);
}

template<typename Mesh>
void test_bgl_OFF(const char* filename)
{
  // read with OFF
  Mesh fg;
  std::ifstream is(filename);
  bool ok = CGAL::read_OFF(is, fg);
  assert(ok);
  assert(num_vertices(fg) != 0 && num_faces(fg) != 0);

  // write with OFF
  {
    ok = CGAL::write_OFF(std::cout, fg);
    assert(ok);

    std::ofstream os("tmp.off");
    ok = CGAL::write_OFF(os, fg);
    assert(ok);

    Mesh fg2;
    ok = CGAL::read_OFF("tmp.off", fg2);
    assert(ok);
    assert(are_equal_meshes(fg, fg2));
  }

  // write with PM
  {
    ok = CGAL::write_polygon_mesh("tmp.off", fg);
    assert(ok);

    Mesh fg2;
    ok = CGAL::read_polygon_mesh("tmp.off", fg2);
    assert(ok);
    assert(are_equal_meshes(fg, fg2));
  }

  // Test [STCN]OFF
  typedef typename boost::property_map<Mesh, CGAL::dynamic_vertex_property_t<Vector> >::type VertexNormalMap;
  typedef typename boost::property_map<Mesh, CGAL::dynamic_vertex_property_t<CGAL::Color> >::type VertexColorMap;
  typedef typename boost::property_map<Mesh, CGAL::dynamic_vertex_property_t<Point_2> >::type VertexTextureMap;
  typedef typename boost::property_map<Mesh, CGAL::dynamic_face_property_t<CGAL::Color> >::type FaceColorMap;

  // COFF
  {
    clear(fg);
    VertexColorMap vcm = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg);
    FaceColorMap fcm = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg);

    ok = CGAL::read_OFF("data/mesh_with_colors.off", fg, CGAL::parameters::vertex_color_map(vcm)
                                                                          .face_color_map(fcm));
    assert(ok);
    assert(num_vertices(fg) == 8 && num_faces(fg) == 4);

    for(const auto v : vertices(fg))
      assert(get(vcm, v) != CGAL::Color());

    for(const auto f : faces(fg))
      assert(get(fcm, f) != CGAL::Color());

    // write with OFF
    {
      ok = CGAL::write_OFF("tmp.off", fg, CGAL::parameters::vertex_color_map(vcm)
                                                           .face_color_map(fcm));
      assert(ok);

      Mesh fg2;
      VertexColorMap vcm2 = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg2);
      FaceColorMap fcm2 = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg2);

      ok = CGAL::read_polygon_mesh("tmp.off", fg2, CGAL::parameters::vertex_color_map(vcm2)
                                                                    .face_color_map(fcm2));
      assert(ok);
      assert(are_equal_meshes(fg, fg2));

      for(const auto v : vertices(fg2))
        assert(get(vcm2, v) != CGAL::Color());

      for(const auto f : faces(fg2))
        assert(get(fcm2, f) != CGAL::Color());
    }

    // write with PM
    {
      ok = CGAL::write_polygon_mesh("tmp.off", fg, CGAL::parameters::vertex_color_map(vcm));
      assert(ok);

      Mesh fg2;
      VertexColorMap vcm2 = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg2);

      ok = CGAL::read_polygon_mesh("tmp.off", fg2, CGAL::parameters::vertex_color_map(vcm2));
      assert(ok);
      assert(are_equal_meshes(fg, fg2));

      for(const auto v : vertices(fg2))
        assert(get(vcm2, v) != CGAL::Color());
    }
  }

  // NOFF
  {
    clear(fg);
    VertexNormalMap vnm = get(CGAL::dynamic_vertex_property_t<Vector>(), fg);

    ok = CGAL::read_OFF("data/mesh_with_normals.off", fg, CGAL::parameters::vertex_normal_map(vnm));
    assert(ok);

    for(const auto v : vertices(fg))
      assert(get(vnm, v) != CGAL::NULL_VECTOR);

    // write with OFF
    {
      std::ofstream os("tmp.off");
      ok = CGAL::write_OFF("tmp.off", fg, CGAL::parameters::vertex_normal_map(vnm));
      assert(ok);

      Mesh fg2;
      VertexNormalMap vnm2 = get(CGAL::dynamic_vertex_property_t<Vector>(), fg2);

      ok = CGAL::read_polygon_mesh("tmp.off", fg2, CGAL::parameters::vertex_normal_map(vnm2));
      assert(ok);
      assert(are_equal_meshes(fg, fg2));

      for(const auto v : vertices(fg2))
        assert(get(vnm2, v) != CGAL::NULL_VECTOR);
    }

    // write with PM
    {
      ok = CGAL::write_polygon_mesh("tmp.off", fg, CGAL::parameters::vertex_normal_map(vnm));
      assert(ok);

      Mesh fg2;
      VertexNormalMap vnm2 = get(CGAL::dynamic_vertex_property_t<Vector>(), fg2);

      ok = CGAL::read_polygon_mesh("tmp.off", fg2, CGAL::parameters::vertex_normal_map(vnm2));
      assert(ok);
      assert(are_equal_meshes(fg, fg2));

      for(const auto v : vertices(fg2))
        assert(get(vnm2, v) != CGAL::NULL_VECTOR);
    }
  }

  // STCNOFF
  {
    clear(fg);
    std::ifstream is("data/full.off");

    VertexNormalMap vnm = get(CGAL::dynamic_vertex_property_t<Vector>(), fg);
    VertexColorMap vcm = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg);
    VertexTextureMap vtm = get(CGAL::dynamic_vertex_property_t<Point_2>(), fg);
    FaceColorMap fcm = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg);

    ok = CGAL::read_OFF(is, fg, CGAL::parameters::vertex_normal_map(vnm)
                                                 .vertex_color_map(vcm)
                                                 .vertex_texture_map(vtm)
                                                 .face_color_map(fcm));
    assert(ok);
    assert(num_vertices(fg) != 0 && num_faces(fg) != 0);

    for(const auto v : vertices(fg))
    {
      assert(get(vnm, v) != CGAL::NULL_VECTOR);
      assert(get(vcm, v) != CGAL::Color());
      assert(get(vtm, v) != Point_2());
    }

    for(const auto f : faces(fg))
      assert(get(fcm, f) != CGAL::Color());

    // write with OFF
    {
      std::ofstream os("tmp.off");
      ok = CGAL::write_OFF("tmp.off", fg, CGAL::parameters::vertex_normal_map(vnm)
                                                           .vertex_color_map(vcm)
                                                           .vertex_texture_map(vtm)
                                                           .face_color_map(fcm));
      assert(ok);

      Mesh fg2;
      VertexNormalMap vnm2 = get(CGAL::dynamic_vertex_property_t<Vector>(), fg2);
      VertexColorMap vcm2 = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg2);
      VertexTextureMap vtm2 = get(CGAL::dynamic_vertex_property_t<Point_2>(), fg2);
      FaceColorMap fcm2 = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg2);

      ok = CGAL::read_polygon_mesh("tmp.off", fg2, CGAL::parameters::vertex_normal_map(vnm2)
                                                                    .vertex_color_map(vcm2)
                                                                    .vertex_texture_map(vtm2)
                                                                    .face_color_map(fcm2));
      assert(ok);
      assert(are_equal_meshes(fg, fg2));

      for(const auto v : vertices(fg2))
      {
        assert(get(vnm2, v) != CGAL::NULL_VECTOR);
        assert(get(vcm2, v) != CGAL::Color());
        assert(get(vtm2, v) != Point_2());
      }

      for(const auto f : faces(fg2))
        assert(get(fcm2, f) != CGAL::Color());
    }

    // write with PM
    {
      ok = CGAL::write_polygon_mesh("tmp.off", fg, CGAL::parameters::vertex_normal_map(vnm)
                                    .vertex_color_map(vcm)
                                    .vertex_texture_map(vtm)
                                    .face_color_map(fcm));
      assert(ok);

      Mesh fg2;
      VertexNormalMap vnm2 = get(CGAL::dynamic_vertex_property_t<Vector>(), fg2);
      VertexColorMap vcm2 = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg2);
      VertexTextureMap vtm2 = get(CGAL::dynamic_vertex_property_t<Point_2>(), fg2);
      FaceColorMap fcm2 = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg2);

      ok = CGAL::read_polygon_mesh("tmp.off", fg2, CGAL::parameters::vertex_normal_map(vnm2)
                                                                    .vertex_color_map(vcm2)
                                                                    .vertex_texture_map(vtm2)
                                                                    .face_color_map(fcm2));
      assert(ok);
      assert(are_equal_meshes(fg, fg2));

      for(const auto v : vertices(fg2))
      {
        assert(get(vnm2, v) != CGAL::NULL_VECTOR);
        assert(get(vcm2, v) != CGAL::Color());
        assert(get(vtm2, v) != Point_2());
      }

      for(const auto f : faces(fg2))
        assert(get(fcm2, f) != CGAL::Color());
    }
  }

  // test wrong inputs
  std::cerr<<"Error text is expected to follow."<<std::endl;
  ok = CGAL::read_OFF("data/mesh_that_doesnt_exist.off", fg);
  assert(!ok);
  ok = CGAL::read_OFF("data/invalid_cut.off", fg); // cut in half
  assert(!ok);
  ok = CGAL::read_OFF("data/invalid_header.off", fg); // wrong header (NOFF but no normals)
  assert(!ok);
  ok = CGAL::read_OFF("data/invalid_nv.off", fg); // wrong number of points
  assert(!ok);
  ok = CGAL::read_OFF("data/sphere.obj", fg);
  assert(!ok);
  ok = CGAL::read_OFF("data/pig.stl", fg);
  assert(!ok);
  std::cerr<<"No more error text from here."<<std::endl;
}

template<typename Mesh>
void test_bgl_OBJ(const std::string filename)
{
  Mesh fg;

  std::ifstream is(filename);
  bool ok = CGAL::read_OBJ(is, fg);
  assert(ok);
  assert(filename != "data/sphere.obj" || (num_vertices(fg) == 162 && num_faces(fg) == 320));

  // write with OBJ
  {
    ok = CGAL::write_OBJ(std::cout, fg);

    std::ofstream os("tmp.obj");
    ok = CGAL::write_OBJ(os, fg);
    assert(ok);

    Mesh fg2;
    ok = CGAL::read_OBJ("tmp.obj", fg2);
    assert(ok);
    assert(are_equal_meshes(fg, fg2));
  }

  // write with PM
  {
    ok = CGAL::write_polygon_mesh("tmp.obj", fg);
    assert(ok);

    Mesh fg2;
    ok = CGAL::read_polygon_mesh("tmp.obj", fg2);
    assert(ok);
    assert(are_equal_meshes(fg, fg2));
  }

  // Test NPs
  typedef typename boost::property_map<Mesh, CGAL::dynamic_vertex_property_t<Vector> >::type VertexNormalMap;

  clear(fg);
  VertexNormalMap vnm = get(CGAL::dynamic_vertex_property_t<Vector>(), fg);

  ok = CGAL::read_OBJ("data/sphere.obj", fg, CGAL::parameters::vertex_normal_map(vnm));
  assert(ok);
  assert(num_vertices(fg) == 162 && num_faces(fg) == 320);

  for(const auto v : vertices(fg))
    assert(get(vnm, v) != CGAL::NULL_VECTOR);

  // write with OBJ
  {
    std::ofstream os("tmp.obj");
    ok = CGAL::write_OBJ("tmp.obj", fg, CGAL::parameters::vertex_normal_map(vnm));
    assert(ok);

    Mesh fg2;
    VertexNormalMap vnm2 = get(CGAL::dynamic_vertex_property_t<Vector>(), fg2);

    ok = CGAL::read_polygon_mesh("tmp.obj", fg2, CGAL::parameters::vertex_normal_map(vnm2));
    assert(ok);
    assert(are_equal_meshes(fg, fg2));

    for(const auto v : vertices(fg2))
      assert(get(vnm2, v) != CGAL::NULL_VECTOR);
  }

  // write with PM
  {
    ok = CGAL::write_polygon_mesh("tmp.obj", fg, CGAL::parameters::vertex_normal_map(vnm));
    assert(ok);

    Mesh fg2;
    VertexNormalMap vnm2 = get(CGAL::dynamic_vertex_property_t<Vector>(), fg2);

    ok = CGAL::read_polygon_mesh("tmp.obj", fg2, CGAL::parameters::vertex_normal_map(vnm2));
    assert(ok);
    assert(are_equal_meshes(fg, fg2));

    for(const auto v : vertices(fg2))
      assert(get(vnm2, v) != CGAL::NULL_VECTOR);
  }

  // test wrong inputs
  ok = CGAL::read_OBJ("data/mesh_that_doesnt_exist.obj", fg);
  assert(!ok);
  ok = CGAL::read_OBJ("data/invalid_cut.obj", fg); // cut in half
  assert(!ok);
  ok = CGAL::read_OBJ("data/invalid_nv.obj", fg); // broken formatting
  assert(!ok);
  ok = CGAL::read_OBJ("data/genus3.obj", fg); // wrong extension
  assert(!ok);
  ok = CGAL::read_OBJ("data/pig.stl", fg);
  assert(!ok);
}

template<class Mesh>
void test_bgl_PLY(const std::string filename,
                  bool binary = false)
{
  Mesh fg;
  std::ifstream is(filename);
  if(binary)
    CGAL::set_mode(is, CGAL::IO::BINARY);

  bool ok = CGAL::read_PLY(is, fg);
  assert(ok);
  assert(filename != "data/colored_tetra.ply" || (num_vertices(fg) == 4 && num_faces(fg) == 4));

  // write with PLY
  {
    ok = CGAL::write_PLY(std::cout, fg);
    assert(ok);

    std::ofstream os("tmp.ply");
    if(binary)
      CGAL::set_mode(os, CGAL::IO::BINARY);

    ok = CGAL::write_PLY(os, fg);
    assert(ok);

    ok = CGAL::write_PLY(os, fg, "test");
    assert(ok);

    Mesh fg2;
    if(binary)
    {
      ok = CGAL::read_PLY("tmp.ply", fg2);
    }
    else
    {
      std::ifstream is(filename);
      ok = CGAL::read_PLY("tmp.ply", fg2);
    }

    assert(ok);
    assert(are_equal_meshes(fg, fg2));
  }

  // test NPs
  typedef typename boost::property_map<Mesh, CGAL::dynamic_vertex_property_t<CGAL::Color> >::type VertexColorMap;
  typedef typename boost::property_map<Mesh, CGAL::dynamic_face_property_t<CGAL::Color> >::type FaceColorMap;

  clear(fg);
  VertexColorMap vcm = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg);
  FaceColorMap fcm = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg);

  std::ifstream is_c("data/colored_tetra.ply"); // ASCII
  ok = CGAL::read_PLY(is_c, fg, CGAL::parameters::vertex_color_map(vcm)
                                                 .face_color_map(fcm));
  assert(ok);
  assert(num_vertices(fg) == 4 && num_faces(fg) == 4);

  for(const auto v : vertices(fg))
    assert(get(vcm, v) != CGAL::Color());

  for(const auto f : faces(fg))
    assert(get(fcm, f) != CGAL::Color());

  // write with PLY
  {
    std::ofstream os("tmp.ply");
    if(binary)
      CGAL::set_mode(os, CGAL::IO::BINARY);

    ok = CGAL::write_PLY("tmp.ply", fg, CGAL::parameters::vertex_color_map(vcm)
                                                         .face_color_map(fcm));
    assert(ok);

    Mesh fg2;
    VertexColorMap vcm2 = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg2);
    FaceColorMap fcm2 = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg2);

    std::ifstream is_rpm("tmp.ply");
    if(binary)
      CGAL::set_mode(is_rpm, CGAL::IO::BINARY);
    ok = CGAL::read_PLY(is_rpm, fg2, CGAL::parameters::vertex_color_map(vcm2)
                                                   .face_color_map(fcm2));
    assert(ok);
    assert(are_equal_meshes(fg, fg2));

    for(const auto v : vertices(fg2))
      assert(get(vcm2, v) != CGAL::Color());

    for(const auto f : faces(fg2))
      assert(get(fcm2, f) != CGAL::Color());
  }

  // write with PM
  {
    std::ofstream os("tmp.ply");
    if(binary)
      CGAL::set_mode(os, CGAL::IO::BINARY);
    ok = CGAL::write_polygon_mesh("tmp.ply", fg, CGAL::parameters::vertex_color_map(vcm)
                                                                  .face_color_map(fcm));
    assert(ok);

    Mesh fg2;
    VertexColorMap vcm2 = get(CGAL::dynamic_vertex_property_t<CGAL::Color>(), fg2);
    FaceColorMap fcm2 = get(CGAL::dynamic_face_property_t<CGAL::Color>(), fg2);

    ok = CGAL::read_polygon_mesh("tmp.ply", fg2, CGAL::parameters::vertex_color_map(vcm2)
                                                                  .face_color_map(fcm2));
    assert(ok);
    assert(are_equal_meshes(fg, fg2));

    for(const auto v : vertices(fg2))
      assert(get(vcm2, v) != CGAL::Color());

    for(const auto f : faces(fg2))
      assert(get(fcm2, f) != CGAL::Color());
  }

  // test wrong inputs
  ok = CGAL::read_PLY("data/mesh_that_doesnt_exist.ply", fg);
  assert(!ok);
  ok = CGAL::read_PLY("data/invalid_cut.ply", fg); // cut in half
  assert(!ok);
  ok = CGAL::read_PLY("data/invalid_nv.ply", fg); // broken formatting
  assert(!ok);
  ok = CGAL::read_PLY("data/cube.off", fg);
  assert(!ok);
  ok = CGAL::read_PLY("data/pig.stl", fg);
  assert(!ok);
}

template<class Mesh>
struct Custom_VPM
{
  typedef Custom_VPM<Mesh>                                      Self;

  typedef typename boost::graph_traits<Mesh>::vertex_descriptor vertex_descriptor;

  typedef vertex_descriptor                                     key_type;
  typedef EPICK::Point_3                                        value_type;
  typedef value_type&                                           reference;
  typedef boost::lvalue_property_map_tag                        category;

  Custom_VPM(std::map<key_type, value_type>& points) : points(points) { }

  friend void put(const Self& m, const key_type& k, const value_type& v) { m.points[k] = value_type(v.x(), v.y(), v.z()); }
  friend reference get(const Self& m, const key_type& k) { return m.points[k]; }

  std::map<key_type, value_type>& points;
};

template<class Mesh>
void test_bgl_STL(const std::string filename)
{
  Mesh fg;

  bool ok = CGAL::read_STL(filename, fg);
  assert(ok);
  ok = CGAL::write_STL("tmp.stl", fg);
  assert(ok);

  clear(fg);

  std::map<typename boost::graph_traits<Mesh>::vertex_descriptor, EPICK::Point_3> cpoints;
  Custom_VPM<Mesh> cvpm(cpoints);

  std::ifstream is(filename);
  ok = CGAL::read_STL(is, fg, CGAL::parameters::vertex_point_map(cvpm));
  assert(ok);
  assert(filename != "data/sphere.stl" || (num_vertices(fg) == 162 && num_faces(fg) == 320));
  assert(filename != "data/sphere.stl" || cpoints.size() == 162);

  // write with STL
  {
    ok = CGAL::write_STL(std::cout, fg, CGAL::parameters::vertex_point_map(cvpm));
    assert(ok);

    std::ofstream os("tmp.stl");
    ok = CGAL::write_STL(os, fg, CGAL::parameters::vertex_point_map(cvpm));
    assert(ok);

    Mesh fg2;
    ok = CGAL::read_STL("tmp.stl", fg2, CGAL::parameters::vertex_point_map(cvpm));
    assert(ok);
    assert(num_vertices(fg) == num_vertices(fg2) && num_faces(fg) == num_faces(fg2));
  }

  // write with PM
  {
    ok = CGAL::write_polygon_mesh("tmp.stl", fg, CGAL::parameters::vertex_point_map(cvpm));
    assert(ok);
    cpoints.clear();
    Mesh fg2;
    ok = CGAL::read_polygon_mesh("tmp.stl", fg2, CGAL::parameters::vertex_point_map(cvpm));
    assert(ok);
    assert(num_vertices(fg) == num_vertices(fg2) && num_faces(fg) == num_faces(fg2));
  }

  // @todo test wrong inputs (see tests of other formats)
}

template<class Mesh>
void test_bgl_GOCAD(const char* filename)
{
  Mesh fg;
  std::ifstream is(filename);
  bool ok = CGAL::read_GOCAD(is, fg);
  assert(ok);
  assert(num_vertices(fg) != 0 && num_faces(fg) != 0);

  is.seekg(0);
  fg.clear();
  clear(fg);
  std::pair<std::string, std::string> name_and_color;
  ok = CGAL::read_GOCAD(is, name_and_color, fg);
  assert(ok);
  assert(num_vertices(fg) != 0 && num_faces(fg) != 0);

  // write with GOCAD
  {
    ok = CGAL::write_GOCAD(std::cout, fg);
    assert(ok);

    std::ofstream os("tmp.ts");
    bool ok = CGAL::write_GOCAD(os, "tetrahedron", fg);
    assert(ok);

    Mesh fg2;
    std::pair<std::string, std::string> cnn;
    ok = CGAL::read_GOCAD("tmp.ts", cnn, fg2);
    assert(ok);
    assert(are_equal_meshes(fg, fg2));
    assert(cnn.first == "tetrahedron");
  }

  // write with PM
  {
    ok = CGAL::write_polygon_mesh("tmp.off", fg);
    assert(ok);

    Mesh fg2;
    ok = CGAL::read_polygon_mesh("tmp.off", fg2);
    assert(ok);
    assert(are_equal_meshes(fg, fg2));
  }

  // test NPs
  typedef typename boost::property_map<Mesh,CGAL::vertex_point_t>::type VertexPointMap;

  VertexPointMap vpm = get(CGAL::vertex_point, fg);

  std::ostringstream out;
  ok = CGAL::write_GOCAD(out, "tetrahedron", fg, CGAL::parameters::vertex_point_map(vpm));
  assert(ok);

  {
    Mesh fg2;
    VertexPointMap vpm2 = get(CGAL::vertex_point, fg2);
    std::istringstream is(out.str());
    std::pair<std::string, std::string> cnn;
    ok = CGAL::read_GOCAD(is, cnn, fg2, CGAL::parameters::vertex_point_map(vpm2));
    assert(ok);
    assert(cnn.second.empty());
    assert(num_vertices(fg2) == 12491);
    assert(num_faces(fg2) == 24191);
  }

  // @todo test wrong inputs (see tests of other formats)
}

#ifdef CGAL_USE_VTK
template<typename Mesh>
void test_bgl_VTP(const char* filename, // @fixme not finished
                  const bool binary = false)
{
  Mesh fg;
  CGAL::make_tetrahedron(Point(0, 0, 0), Point(1, 1, 0),
                         Point(2, 0, 1), Point(3, 0, 0), fg);

  std::ofstream os("tetrahedron.vtp");
  bool ok = CGAL::write_VTP(os, fg, CGAL::parameters::use_binary_mode(binary));
  assert(ok);

  Mesh fg2;
  ok = CGAL::read_polygon_mesh("tetrahedron.vtp", fg2);
  assert(ok);
  assert(are_equal_meshes(fg, fg2));
}

template<>
void test_bgl_VTP<Polyhedron>(const char* filename,
                              const bool binary)
{
 //todo same tests as the others.
}
#endif // CGAL_USE_VTK

int main(int argc, char** argv)
{
  // OFF
  /*
  const char* off_file = (argc > 1) ? argv[1] : "data/prim.off";
  test_bgl_OFF<Polyhedron>(off_file);
  test_bgl_OFF<SM>(off_file);
  test_bgl_OFF<LCC>(off_file);
#ifdef CGAL_USE_OPENMESH
  test_bgl_OFF<OMesh>(off_file);
#endif
  // OBJ
  const char* obj_file = (argc > 2) ? argv[2] : "data/sphere.obj";
  test_bgl_OBJ<Polyhedron>(obj_file);
  test_bgl_OBJ<SM>(obj_file);
  test_bgl_OBJ<LCC>(obj_file);
#ifdef CGAL_USE_OPENMESH
  test_bgl_OBJ<OMesh>(obj_file);
#endif

  // PLY
  const char* ply_file_ascii = (argc > 3) ? argv[3] : "data/colored_tetra.ply";
  test_bgl_PLY<Polyhedron>(ply_file_ascii, false);
  test_bgl_PLY<SM>(ply_file_ascii, false);

  const char* ply_file = (argc > 3) ? argv[3] : "data/colored_tetra.ply";
  test_bgl_PLY<Polyhedron>(ply_file, true);
  test_bgl_PLY<SM>(ply_file, true);

  // STL
  const char* stl_file = (argc > 4) ? argv[4] : "data/pig.stl";
  test_bgl_STL<Polyhedron>(stl_file);
  test_bgl_STL<SM>(stl_file);
  test_bgl_STL<LCC>(stl_file);
#ifdef CGAL_USE_OPENMESH
  test_bgl_STL<OMesh>(stl_file);
#endif

  // GOCAD
  const char* gocad_file = (argc > 5) ? argv[5] : "data/2016206_MHT_surface.ts";
  test_bgl_GOCAD<Polyhedron>(gocad_file);
  test_bgl_GOCAD<SM>(gocad_file);
 test_bgl_GOCAD<LCC>(gocad_file);
#ifdef CGAL_USE_OPENMESH
  test_bgl_GOCAD<OMesh>(gocad_file);
#endif
*/
  // VTP
#ifdef CGAL_USE_VTK
  const char* vtp_file = (argc > 6) ? argv[6] : "data/prim.off"; // @fixme put a VTP file

  test_bgl_VTP<Polyhedron>(vtp_file, false);
  test_bgl_VTP<SM>(vtp_file, false);
  test_bgl_VTP<LCC>(vtp_file, false);

  test_bgl_VTP<Polyhedron>(vtp_file, true);
  test_bgl_VTP<SM>(vtp_file, true);
  test_bgl_VTP<LCC>(vtp_file, true);
#endif
  return EXIT_SUCCESS;
}
