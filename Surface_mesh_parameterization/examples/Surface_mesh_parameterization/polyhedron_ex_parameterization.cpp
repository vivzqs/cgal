// ----------------------------------------------------------------------------
// USAGE EXAMPLES
// ----------------------------------------------------------------------------

//----------------------------------------------------------
// Discrete Conformal Map parameterization
// circle border
// OpenNL solver
// output is a eps map
// input file is mesh.off
//----------------------------------------------------------
// polyhedron_ex_parameterization -t conformal -b circle -o eps mesh.off mesh.eps

//----------------------------------------------------------
// floater parameterization
// square border
// TAUCS solver
// output is a eps map
// input file is mesh.off
//----------------------------------------------------------
// polyhedron_ex_parameterization -t floater -b square -s taucs -o eps mesh.off mesh.eps

//----------------------------------------------------------
// Least Squares Conformal Maps parameterization
// two pinned vertices (automatically picked)
// OpenNL solver
// output is a .obj
// input file is mesh.off
//----------------------------------------------------------
// polyhedron_ex_parameterization -t lscm -b 2pts -o obj mesh.off mesh.obj


#include <CGAL/Cartesian.h>
#include <CGAL/Timer.h>
#include <CGAL/parameterize.h>
#include <CGAL/Parameterization_mesh_patch_3.h>
#include <CGAL/Circular_border_parameterizer_3.h>
#include <CGAL/Square_border_parameterizer_3.h>
#include <CGAL/Two_vertices_parameterizer_3.h>
#include <CGAL/Barycentric_mapping_parameterizer_3.h>
#include <CGAL/Discrete_conformal_map_parameterizer_3.h>
#include <CGAL/Discrete_authalic_parameterizer_3.h>
#include <CGAL/Mean_value_coordinates_parameterizer_3.h>
#include <CGAL/LSCM_parameterizer_3.h>
#include <CGAL/Parameterization_mesh_feature_extractor.h>

#include <OpenNL/linear_solver.h>
#ifdef CGAL_USE_TAUCS
    #include <CGAL/Taucs_solver_traits.h>
#endif

#include "Polyhedron_ex.h"
#include "Mesh_cutter.h"
#include "Parameterization_polyhedron_adaptor_ex.h"

#include <iostream>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include <cassert>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#ifdef WIN32
    #include <Windows.h>
#endif

// ----------------------------------------------------------------------------
// Private types
// ----------------------------------------------------------------------------

typedef Polyhedron_ex                                       Polyhedron;

// Mesh adaptors
typedef Parameterization_polyhedron_adaptor_ex              Parameterization_polyhedron_adaptor;
typedef CGAL::Parameterization_mesh_patch_3<Parameterization_polyhedron_adaptor>
                                                            Mesh_patch_polyhedron;

// Type describing a border or seam as a vertex list
typedef std::list<Parameterization_polyhedron_adaptor::Vertex_handle>
                                                            Seam;


// ----------------------------------------------------------------------------
// Private functions
// ----------------------------------------------------------------------------

// Cut the mesh to make it homeomorphic to a disk
// or extract a region homeomorphic to a disc.
// Return the border of this region (empty on error)
//
// CAUTION:
// This method is provided "as is". It is very buggy and simply part of this example.
// Developers using this package should implement a more robust cut algorithm!
static Seam cut_mesh(Parameterization_polyhedron_adaptor& mesh_adaptor)
{
    // Helper class to compute genus or extract borders
    typedef CGAL::Parameterization_mesh_feature_extractor<Parameterization_polyhedron_adaptor_ex>
                                            Mesh_feature_extractor;
    typedef Mesh_feature_extractor::Border  Border;
    typedef Mesh_cutter::Backbone           Backbone;

    Seam seam;              // returned list

    // Get refererence to Polyhedron_3 mesh
    Polyhedron& mesh = mesh_adaptor.get_adapted_mesh();

    // Extract mesh borders and compute genus
    Mesh_feature_extractor feature_extractor(mesh_adaptor);
    int nb_borders = feature_extractor.get_nb_borders();
    int genus = feature_extractor.get_genus();

    // If mesh is a topological disk
    if (genus == 0 && nb_borders > 0)
    {
        // Pick the longest border
        seam = feature_extractor.get_longest_border();
    }
    else // if mesh is NOT a topological disk, create a virtual cut
    {
        Backbone seamingBackbone;           // result of cutting
        Backbone::iterator he;

        // Compute a cutting path that makes the mesh a "virtual" topological disk
        mesh.compute_facet_centers();
        Mesh_cutter cutter(mesh);
        if (genus == 0)
        {
            // no border, we need to cut the mesh
            assert (nb_borders == 0);
            cutter.cut(seamingBackbone);    // simple cut
        }
        else // genus > 0 -> cut the mesh
        {
            cutter.cut_genus(seamingBackbone);
        }

        // The Mesh_cutter class is quite buggy
        // => we check that seamingBackbone is valid
        //
        // 1) Check that seamingBackbone is not empty
        if (seamingBackbone.begin() == seamingBackbone.end())
            return seam;                    // return empty list
        //
        // 2) Check that seamingBackbone is a loop and
        //    count occurences of seam halfedges
        mesh.tag_halfedges(0);              // Reset counters
        for (he = seamingBackbone.begin(); he != seamingBackbone.end(); he++)
        {
            // Get next halfedge iterator (looping)
            Backbone::iterator next_he = he;
            next_he++;
            if (next_he == seamingBackbone.end())
                next_he = seamingBackbone.begin();

            // Check that seamingBackbone is a loop: check that
            // end of current HE == start of next one
            if ((*he)->vertex() != (*next_he)->opposite()->vertex())
                return seam;                // return empty list

            // Increment counter (in "tag" field) of seam halfedges
            (*he)->tag( (*he)->tag()+1 );
        }
        //
        // 3) check that the seamingBackbone is a two-way list
        for (he = seamingBackbone.begin(); he != seamingBackbone.end(); he++)
        {
            // Counter of halfedge and opposite halfedge must be 1
            if ((*he)->tag() != 1 || (*he)->opposite()->tag() != 1)
                return seam;                // return empty list
        }

        // Convert list of halfedges to a list of vertices
        for (he = seamingBackbone.begin(); he != seamingBackbone.end(); he++)
            seam.push_back((*he)->vertex());
    }

    return seam;
}

// Call appropriate parameterization method based on command line parameters
template<
    class ParameterizationMesh_3,   // 3D surface
    class GeneralSparseLinearAlgebraTraits_d,
                                    // Traits class to solve a general sparse linear system
    class SymmetricSparseLinearAlgebraTraits_d
                                    // Traits class to solve a symmetric sparse linear system
>
typename CGAL::Parameterizer_traits_3<ParameterizationMesh_3>::Error_code
parameterize(ParameterizationMesh_3& mesh,  // Mesh parameterization adaptor
             const std::string& type,              // type of parameterization (see usage)
             const std::string& border)            // type of border parameterization (see usage)
{
    typename CGAL::Parameterizer_traits_3<ParameterizationMesh_3>::Error_code err;

    if ( (type == std::string("floater"))  && (border == std::string("circle")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Mean_value_coordinates_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Circular_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("floater")) && (border == std::string("square")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Mean_value_coordinates_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Square_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("barycentric")) && (border == std::string("circle")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Barycentric_mapping_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Circular_border_uniform_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("barycentric")) && (border == std::string("square")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Barycentric_mapping_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Square_border_uniform_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("conformal")) && (border == std::string("circle")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Discrete_conformal_map_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Circular_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("conformal")) && (border == std::string("square")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Discrete_conformal_map_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Square_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("authalic")) && (border == std::string("circle")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Discrete_authalic_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Circular_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("authalic")) && (border == std::string("square")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::Discrete_authalic_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Square_border_arc_length_parameterizer_3<ParameterizationMesh_3>,
                GeneralSparseLinearAlgebraTraits_d
            >());
    }
    else if ( (type == std::string("lscm")) && (border == std::string("2pts")) )
    {
        err = CGAL::parameterize(
            mesh,
            CGAL::LSCM_parameterizer_3<
                ParameterizationMesh_3,
                CGAL::Two_vertices_parameterizer_3<ParameterizationMesh_3>,
                SymmetricSparseLinearAlgebraTraits_d
            >());
    }
    else
    {
        std::cerr << "FATAL ERROR: invalid parameters combination " << type << " + " << border << std::endl;
        err = CGAL::Parameterizer_traits_3<ParameterizationMesh_3>::ERROR_WRONG_PARAMETER;
    }

    return err;
}


// ----------------------------------------------------------------------------
// Usage
// ----------------------------------------------------------------------------

// Parameters description for Options library
static const char *  optv[] =
{
    //     '|' -- indicates that the option takes NO argument;
    //     '?' -- indicates that the option takes an OPTIONAL argument;
    //     ':' -- indicates that the option takes a REQUIRED argument;
    //     '*' -- indicates that the option takes 0 or more arguments;
    //     '+' -- indicates that the option takes 1 or more arguments;

    "t:type <string>", // -t or --type
    // -t    floater     -> Floater Mean Value Coordinates (default)
    //       conformal   -> Discrete Conformal Map
    //       barycentric -> Tutte Barycentric Mapping (weight = 1)
    //       authalic    -> Discrete Authalic Parameterization (weak area-preserving)
    //       lscm        -> Least Squares Conformal Maps


    "b:border <string>", // -b or --border (for fixed border param.)
    // -b circle        -> map mesh border onto a circle
    //    square        -> map mesh border onto a square
    //    2pts          -> pin 2 points

    "s:solver <string>", // -s or --solver
    // -s opennl        -> OpenNL solver (GPL)
    //    taucs         -> TAUCS solver

    "o:output <string>", // -o or --output
    // -o eps   -> eps map       (-o eps file.off > file.eps)
    //    obj   -> Wavefront obj (-o obj file.off > file.obj)

    NULL
} ;

// Parameters description for usage
static const char * usage = "off-input-file [output-file]\n\
where type is:   floater (default), conformal, barycentric, authalic or lscm\n\
      border is: circle (default), square or 2pts\n\
      solver is: opennl (default) or taucs\n\
      output is: eps or obj (default is no output)";


// ----------------------------------------------------------------------------
// main()
// ----------------------------------------------------------------------------

int main(int argc, char * argv[])
{
#if _WIN32_WINNT >= 0x0400
    // Trick to be prompted by VisualC++ debugger when an assertion
    // fails even though we use NON debug runtime libraries
    // (the only ones compatible with TAUCS)
    if (IsDebuggerPresent())
        _set_error_mode(_OUT_TO_MSGBOX);
#endif

    CGAL::Timer total_timer;
    total_timer.start();

    std::cerr << "PARAMETERIZATION" << std::endl;

    // options
    std::string type;       // default: Floater param
    std::string border;      // default: circular border param.
    std::string solver;      // default: OpenNL solver
    std::string input;            // default: no output
    std::string output;            // default: no output

    // misc
    char  optchar;
    const char *optarg;
    int  errors = 0;
    int  npos = 0;
try {
    po::options_description desc("Allowed options");
    desc.add_options()
      ("help,h", "produce help message.")
      ("border,b", po::value<std::string>(&border)->default_value("circle"),
       "circle, square or 2pts")
      ("type,t", po::value<std::string>(&type)->default_value("floater"),
       "floater (default), conformal, barycentric, authalic or lscm")
      ("solver,s", po::value<std::string>(&solver)->default_value("opennl"),
       "opennl (default) or taucs")
      ("input,i", po::value<std::string>(&input)->default_value("mesh-in.off"),
       "polyhedral mesh")
      ("output,o", po::value<std::string>(&output)->default_value("mesh-out.eps"),
       "eps or obj")
      ;


    po::positional_options_description p;
    p.add("input", 1);
    p.add("output", 2);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
	      options(desc).positional(p).run(), vm);

    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 1;
    }
  }
  catch(std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  catch(...) {
    std::cerr << "Exception of unknown type!\n";
  }


    //***************************************
    // Read the mesh
    //***************************************

    CGAL::Timer task_timer;
    task_timer.start();

    // Read the mesh
    std::ifstream stream(input.c_str());
    Polyhedron mesh;
    stream >> mesh;
    if(!stream || !mesh.is_valid() || mesh.empty())
    {
        std::cerr << "FATAL ERROR: cannot read OFF file " << input << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << "Read file " << input << ": "
              << task_timer.time() << " seconds "
              << "(" << mesh.size_of_facets() << " facets, "
              << mesh.size_of_vertices() << " vertices)" << std::endl;
    task_timer.reset();

    //***************************************
    // Create mesh adaptor
    //***************************************

    // The Surface_mesh_parameterization package needs an adaptor to handle Polyhedron_ex meshes
    Parameterization_polyhedron_adaptor mesh_adaptor(mesh);

    // The parameterization methods support only meshes that
    // are topological disks => we need to compute a cutting path
    // that makes the mesh a "virtual" topological disk
    //
    // 1) Cut the mesh
    Seam seam = cut_mesh(mesh_adaptor);
    if (seam.empty())
    {
        std::cerr << "FATAL ERROR: an unexpected error occurred while cutting the shape" << std::endl;
        return EXIT_FAILURE;
    }
    //
    // 2) Create adaptor that virtually "cuts" a patch in a Polyhedron_ex mesh
    Mesh_patch_polyhedron   mesh_patch(mesh_adaptor, seam.begin(), seam.end());
    if (!mesh_patch.is_valid())
    {
        std::cerr << "FATAL ERROR: non manifold shape or invalid cutting" << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << "Mesh cutting: " << task_timer.time() << " seconds." << std::endl;
    task_timer.reset();

    //***************************************
    // switch parameterization
    //***************************************

    std::cerr << "Parameterization..." << std::endl;

    // Defines the error codes
    typedef CGAL::Parameterizer_traits_3<Mesh_patch_polyhedron> Parameterizer;
    Parameterizer::Error_code err;

    if (solver == std::string("opennl"))
    {
        err = parameterize<Mesh_patch_polyhedron,
                           OpenNL::DefaultLinearSolverTraits<double>,
                           OpenNL::SymmetricLinearSolverTraits<double>
                          >(mesh_patch, type, border);
        if (err != Parameterizer::OK)
            std::cerr << "FATAL ERROR: " << Parameterizer::get_error_message(err) << std::endl;
    }
    else if (solver == std::string("taucs"))
    {
#ifdef CGAL_USE_TAUCS
        err = parameterize<Mesh_patch_polyhedron,
                           CGAL::Taucs_solver_traits<double>,
                           CGAL::Taucs_symmetric_solver_traits<double>
                          >(mesh_patch, type, border);
        if (err != Parameterizer::OK)
            std::cerr << "FATAL ERROR: " << Parameterizer::get_error_message(err) << std::endl;
#else
        std::cerr << "FATAL ERROR: TAUCS is not installed" << std::endl;
        err = Parameterizer::ERROR_WRONG_PARAMETER;
#endif
    }
    else
    {
        std::cerr << "FATAL ERROR: invalid solver parameter " << solver << std::endl;
        err = Parameterizer::ERROR_WRONG_PARAMETER;
    }

    std::cerr << "Parameterization: " << task_timer.time() << " seconds." << std::endl;
    task_timer.reset();

    //***************************************
    // Output
    //***************************************

    // Save mesh
    if (err == Parameterizer::OK)
    {
        if(output.substr(output.length()-4) == std::string(".eps"))
        {
            // write Postscript file
            if ( ! mesh.write_file_eps(output.c_str()) )
            {
                std::cerr << "FATAL ERROR: cannot write file " << output << std::endl;
                return EXIT_FAILURE;
            }
        }
        else if(output.substr(output.length()-4) == std::string(".obj"))
        {
            // write Wavefront obj file
            if ( ! mesh.write_file_obj(output.c_str()) )
            {
                std::cerr << "FATAL ERROR: cannot write file " << output << std::endl;
                return EXIT_FAILURE;
            }
        }
        else
        {
            std::cerr << "FATAL ERROR: cannot write format " << output << std::endl;
            err = Parameterizer::ERROR_WRONG_PARAMETER;
        }

        std::cerr << "Write file " << output << ": "
                  << task_timer.time() << " seconds " << std::endl;
    }

    return (err == Parameterizer::OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
