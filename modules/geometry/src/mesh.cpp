/**
 *  @file   mesh.cpp
 *  @brief  3D Mesh container
 *
 *  @author Christophe Ecabert
 *  @date   02/08/16
 *  Copyright © 2016 Christophe Ecabert. All rights reserved.
 */

#include <assert.h>
#include <fstream>
#include <sstream>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

#include "chlib/geometry/mesh.hpp"

/**
 *  @namespace  CHLib
 *  @brief      Chris dev space
 */
namespace CHLib {

#pragma mark -
#pragma mark Type definition

#pragma mark -
#pragma mark Initialization

/*
 *  @name
 *  @fn
 *  @brief  Constructor
 */
template<typename T>
Mesh<T>::Mesh(void) : bbox_is_computed_(false) {
}

/*
 *  @name Mesh
 *  @fn Mesh(const std::string& filename,
 const LTS5::OGLProgram& program)
 *  @brief  Load mesh from supported file :
 *            .obj, .ply, .tri
 */
template<typename T>
Mesh<T>::Mesh(const std::string& filename) : bbox_is_computed_(false){
  if (this->Load(filename)) {
    std::cout << "Error while loading mesh from file : " + filename << std::endl;
  }
}

/*
 *  @name Mesh  @fn Meshid)
 *  @brief  Destructor
 */
template<typename T>
Mesh<T>::~Mesh(void) {
}

/*
 *  @name Load
 *  @fn int Load(const std::string& filename)
 *  @brief  Load mesh from supported file :
 *            .obj, .ply, .tri
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::Load(const std::string& filename) {
  // Load data
  int err = -1;
  size_t pos = filename.rfind(".");
  if (pos != std::string::npos) {
    // Ensure empty containter
    vertex_.clear();
    normal_.clear();
    tex_coord_.clear();
    tri_.clear();
    vertex_con_.clear();
    //memset(bbox_, 0, sizeof(bbox_));
    std::string ext = filename.substr(pos + 1, filename.length());
    FileExt file_ext = this->HashExt(ext);
    switch (file_ext) {
        // OBJ
      case kObj: {
        err = this->LoadOBJ(filename);
      }
        break;
        // PLY
      case kPly: {
        err = this->LoadPLY(filename);
      }
        break;
        // Triangulation only
      case kTri: {
        err = this->LoadTri(filename);
      }
        break;

        case kUndef:
      default:  std::cout << "Error, unsported extension type : " << ext;
        std::cout << std::endl;
        err = -1;
        break;
    }
    if (!err && (file_ext != FileExt::kTri)) {
      this->BuildConnectivity();
    }
  }
  if (!err && !bbox_is_computed_) {
    ComputeBoundingBox();
  }
  return err;
}

/*
 *  @name Save
 *  @fn int Save(const std::string& filename)
 *  @brief  Save mesh to supported file format:
 *            .ply
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::Save(const std::string& filename) {
  // Load data
  int err = -1;
  size_t pos = filename.rfind(".");
  if (pos != std::string::npos) {
    std::string ext = filename.substr(pos + 1, filename.length());
    FileExt file_ext = this->HashExt(ext);
    switch (file_ext) {
        // PLY
      case kPly: {
        err = this->SavePLY(filename);
      }
        break;
        // OBJ
      case kObj: {
        err = this->SaveOBJ(filename);
      }
        break;

        break;
        // Undef
        case kUndef:
      default:  std::cout << "Error, unsported extension type : " << ext;
        std::cout << std::endl;
        err = -1;
        break;
    }
  }
  return err;
}

/*
 *  @name BuildConnectivity
 *  @fn void BuildConnectivity(void)
 *  @brief  Construct vertex connectivity (vertex connection) used later
 *          in normal computation
 */
template<typename T>
void Mesh<T>::BuildConnectivity(void) {
  // Init outter containter
  assert(vertex_.size() != 0 && tri_.size() != 0);
  vertex_con_.clear();
  vertex_con_ = std::vector<std::vector<int>>(vertex_.size(),
                                              std::vector<int>(0));
  // Loop over all triangle
  const int n_tri = static_cast<int>(tri_.size());
  for (int i = 0; i < n_tri; ++i) {
    int* tri_idx_ptr = &(tri_[i].x_);
    for (int e = 0; e < 3; ++e) {
      int idx_in = tri_idx_ptr[e];
      int idx_out_1 = tri_idx_ptr[(e + 1) % 3];
      int idx_out_2 = tri_idx_ptr[(e + 2) % 3];
      // Add to connectivity list
      vertex_con_[idx_in].push_back(idx_out_1);
      vertex_con_[idx_in].push_back(idx_out_2);
    }
  }
}

/*
 *  @name HashExt
 *  @fn FileExt HashExt(const std::string& ext)
 *  @brief  Provide the type of extension
 *  @param[in]  ext Name of the extension
 *  @return Hash of the given extension if know by the object
 */
template<typename T>
typename Mesh<T>::FileExt Mesh<T>::HashExt(const std::string& ext) {
  FileExt fext = kUndef;
  if (ext == "obj") {
    fext = kObj;
  } else if (ext == "ply") {
    fext = kPly;
  } else if (ext == "tri") {
    fext = kTri;
  }
  return fext;
}

/*
 *  @name LoadOBJ
 *  @fn int LoadOBJ(const std::string& path)
 *  @brief  Load mesh from .obj file
 *  @param[in]  path  Path to obj file
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::LoadOBJ(const std::string& path) {
  int error = -1;
  std::ifstream stream(path, std::ios_base::in);
  if (stream.is_open()) {
    // Init bbox
    bbox_.min_.x_ = std::numeric_limits<T>::max();
    bbox_.max_.x_ = std::numeric_limits<T>::lowest();
    bbox_.min_.y_ = std::numeric_limits<T>::max();
    bbox_.max_.y_ = std::numeric_limits<T>::lowest();
    bbox_.min_.z_ = std::numeric_limits<T>::max();
    bbox_.max_.z_ = std::numeric_limits<T>::lowest();
    // Start to read
    std::string line, key;
    std::stringstream str_stream;
    Vertex vertex;
    Normal normal;
    TCoord tcoord;
    Triangle tri;
    while (!stream.eof()) {
      // Scan line
      std::getline(stream, line);
      if (!line.empty()) {
        str_stream.str(line);
        str_stream >> key;
        if (key == "v") {
          // Vertex
          str_stream >> vertex;
          vertex_.push_back(vertex);
          // Compute boundary box
          bbox_.min_.x_ = bbox_.min_.x_ < vertex.x_ ? bbox_.min_.x_ : vertex.x_;
          bbox_.max_.x_ = bbox_.max_.x_ > vertex.x_ ? bbox_.max_.x_ : vertex.x_;
          bbox_.min_.y_ = bbox_.min_.y_ < vertex.y_ ? bbox_.min_.y_ : vertex.y_;
          bbox_.max_.y_ = bbox_.max_.y_ > vertex.y_ ? bbox_.max_.y_ : vertex.y_;
          bbox_.min_.z_ = bbox_.min_.z_ < vertex.z_ ? bbox_.min_.z_ : vertex.z_;
          bbox_.max_.z_ = bbox_.max_.z_ > vertex.z_ ? bbox_.max_.z_ : vertex.z_;
        } else if (key == "vn") {
          // Normal
          str_stream >> normal;
          normal_.push_back(normal);
        } else if (key == "vt") {
          // Texture coordinate
          str_stream >> tcoord;
          tex_coord_.push_back(tcoord);
        } else if (key == "f") {
          // Faces
          str_stream >> tri;
          tri -= 1;
          tri_.push_back(tri);
        }
        str_stream.clear();
      }
    }
    bbox_.center_ = (bbox_.min_ + bbox_.max_) * T(0.5);
    bbox_is_computed_ = true;
    error = 0;
  }
  return error;
}

/*
 *  @name LoadPLY
 *  @fn int LoadPLY(const std::string& path)
 *  @brief  Load mesh from .obj file
 *  @param[in]  path  Path to ply file
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::LoadPLY(const std::string& path) {
  int error = -1;
  // TODO: Need to be implemented !
  return error;
}

/*
 *  @name SaveOBJ
 *  @fn int SaveOBJ(const std::string path) const
 *  @brief SAve mesh to a .obj file
 *  @param[in]  path  Path to .obj file
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::SaveOBJ(const std::string& path) {
  int err = -1;
  std::ofstream out_stream(path, std::ios_base::out);
  if (out_stream.is_open()) {
    // Header
    out_stream << "# wavefront obj file written by LTS5 c++ library" << std::endl;

    // Vertex
    if (vertex_.size() > 0) {
      size_t n_vert = vertex_.size();
      for (size_t i = 0; i < n_vert; ++i) {
        const Vertex& v = vertex_[i];
        out_stream << "f " << v.x_ << " " << v.y_ << " " << v.z_ << std::endl;
      }
    }
    // Normal
    if (normal_.size() > 0) {
      size_t n_norm = normal_.size();
      for (size_t i = 0; i < n_norm; ++i) {
        const Normal& n = normal_[i];
        out_stream << "vn " << n.x_ << " " << n.y_ << " " << n.z_ << std::endl;
      }
    }
    // Texture coordinate
    if (tex_coord_.size() > 0) {
      size_t n_tcoord = tex_coord_.size();
      for (size_t i = 0; i < n_tcoord; ++i) {
        const TCoord& tc = tex_coord_[i];
        out_stream << "vt " << tc.x_ << " " << tc.y_ << std::endl;
      }
    }
    // Tri
    if (tri_.size() > 0) {
      size_t n_tri = tri_.size();
      for (size_t i = 0; i < n_tri; ++i) {
        const Triangle & tri = tri_[i];
        out_stream << "f " << tri.x_ << " " << tri.y_ << " " << tri.z_ << std::endl;
      }
    }
    // Done
    err = out_stream.good() ? 0 : -1;
    out_stream.close();
  }
  return err;
}

/*
 *  @name SavePLY
 *  @fn int SavePLY(const std::string& path) const
 *  @brief SAve mesh to a .ply file
 *  @param[in]  path  Path to .ply file
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::SavePLY(const std::string& path) const {
  int error = -1;
  // TODO: Need to be implemented !
  return error;
}

/*
 *  @name LoadTri
 *  @fn int LoadTri(const std::string& path)
 *  @brief  Load mesh triangulation from .tri file
 *  @param[in]  path  Path to .tri file
 *  @return -1 if error, 0 otherwise
 */
template<typename T>
int Mesh<T>::LoadTri(const std::string& path) {
  int error = -1;
  FILE* file_id = fopen(path.c_str(), "r");
  if(file_id) {
    const int LINE_SIZE = 1024;
    char buffer[LINE_SIZE];
    int n_line = 0;
    bool ok = true;
    // Start to loop over
    while(ok && fgets(buffer, LINE_SIZE, file_id) != NULL) {
      n_line++;
      char* line_ptr = buffer;
      char* end_ptr = buffer + strlen(buffer);
      // Find first non-white space character
      while(isspace(*line_ptr) && line_ptr < end_ptr) {line_ptr++;}
      // Check command, (expected to be 'f')
      const char* cmd = line_ptr;
      // Skip over non-whitespace
      while( !isspace(*line_ptr) && line_ptr < end_ptr) {line_ptr++;}
      //  terminate command
      if (line_ptr < end_ptr) {
        *line_ptr = '\0';
        line_ptr++;
      }
      // in the OBJ format the first characters determine how to interpret
      // the line:
      if (strcmp(cmd, "f") == 0) {
        Triangle tri;
        int* tri_ptr = &tri.x_;
        int vertex_cnt = 0;

        while (ok && line_ptr < end_ptr) {
          //  find the first non-whitespace character
          while (isspace(*line_ptr) && line_ptr < end_ptr) { line_ptr++; }
          //  there is still data left on this line
          if (line_ptr < end_ptr) {
            int vertex_idx, t_coord_idx, normal_idx;
            if (sscanf(line_ptr,
                       "%d",
                       &vertex_idx) == 1) {
              // Shape polygon
              tri_ptr[vertex_cnt] = vertex_idx - 1;
              vertex_cnt++;
            } else if (sscanf(line_ptr,
                              "%d/%d",
                              &vertex_idx,
                              &t_coord_idx) == 2) {
              // Shape polygon
              tri_ptr[vertex_cnt] = vertex_idx - 1;
              vertex_cnt++;
            } else if (sscanf(line_ptr,
                              "%d/%d/%d",
                              &vertex_idx,
                              &t_coord_idx,
                              &normal_idx) == 3) {
              // Shape polygon
              tri_ptr[vertex_cnt] = vertex_idx - 1;
              vertex_cnt++;
            } else {
              std::cout << "Error reading 'f' at line : " <<
              n_line << std::endl;
              ok = false;
            }
            //  skip over what we just read
            //  (find the first whitespace character)
            while (!isspace(*line_ptr) && line_ptr < end_ptr) { line_ptr++; }
          }
        }
        assert(vertex_cnt == 3);
        tri_.push_back(tri);
      }
    }
    fclose(file_id);
    error = ok ? 0 : -1;
  } else {
    std::cerr << "Can't load triangulation file !!!" << std::endl;
  }
  return error;
}

#pragma mark -
#pragma mark Usage

  /*
   *  @name ComputeVertexNormal
   *  @fn void ComputeVertexNormal(void)
   *  @brief  Compute normal for each vertex in the object
   */
template<typename T>
void Mesh<T>::ComputeVertexNormal(void) {
  // Loop over all vertex
  assert(vertex_con_.size() > 0);
  const int n_vert = static_cast<int>(vertex_.size());
  normal_.resize(n_vert, Mesh::Normal());
#if defined(__APPLE__)
  dispatch_apply(n_vert,
                 dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,
                                           (unsigned long)NULL), ^(size_t v) {
#else
  #pragma omp parallel for
  for (int v = 0; v < n_vert; ++v) {
#endif
   // Loop over all connect vertex
   std::vector<int>& conn = vertex_con_[v];
   const Vertex& A = vertex_[v];
   const int n_conn = static_cast<int>(conn.size());
   Normal weighted_n;
   for (int j = 0; j < n_conn; j += 2) {
     const Vertex& B = vertex_[conn[j]];
     const Vertex& C = vertex_[conn[j+1]];
     // Define edges AB, AC
     Edge AB = B - A;
     Edge AC = C - A;
     // Compute surface's normal (triangle ABC)
     Normal n = AB ^ AC;
     n.Normalize();
     // Stack each face contribution and weight with angle
     AB.Normalize();
     AC.Normalize();
     const T angle = std::acos(AB * AC);
     weighted_n += (n * angle);
   }
   // normalize and set
   weighted_n.Normalize();
   normal_[v] = weighted_n;
 }
#if defined(__APPLE__)
  );
#endif
}

/*
 *  @name ComputeBoundingBox
 *  @fn void ComputeBoundingBox(void)
 *  @brief  Compute the bounding box of the mesh
 */
template<typename T>
void Mesh<T>::ComputeBoundingBox(void) {
 // Init bbox
 bbox_.min_.x_ = std::numeric_limits<T>::max();
 bbox_.max_.x_ = std::numeric_limits<T>::lowest();
 bbox_.min_.y_ = std::numeric_limits<T>::max();
 bbox_.max_.y_ = std::numeric_limits<T>::lowest();
 bbox_.min_.z_ = std::numeric_limits<T>::max();
 bbox_.max_.z_ = std::numeric_limits<T>::lowest();

 for (auto v_it = this->vertex_.begin(); v_it != this->vertex_.end(); ++v_it) {
   // Compute boundary box
   bbox_.min_.x_ = bbox_.min_.x_ < v_it->x_ ? bbox_.min_.x_ : v_it->x_;
   bbox_.max_.x_ = bbox_.max_.x_ > v_it->x_ ? bbox_.max_.x_ : v_it->x_;
   bbox_.min_.y_ = bbox_.min_.y_ < v_it->y_ ? bbox_.min_.y_ : v_it->y_;
   bbox_.max_.y_ = bbox_.max_.y_ > v_it->y_ ? bbox_.max_.y_ : v_it->y_;
   bbox_.min_.z_ = bbox_.min_.z_ < v_it->z_ ? bbox_.min_.z_ : v_it->z_;
   bbox_.max_.z_ = bbox_.max_.z_ > v_it->z_ ? bbox_.max_.z_ : v_it->z_;
 }
 bbox_.center_ = (bbox_.min_ + bbox_.max_) * T(0.5);
 bbox_is_computed_ = true;
}

#pragma mark -
#pragma mark Declaration

/** Float Mesh */
template class Mesh<float>;
/** Double Mesh */
template class Mesh<double>;


}  // namespace CHLib