//=============================================================================
// Copyright (C) 2011-2018 The pmp-library developers
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//=============================================================================

#include <pmp/algorithms/TriangleKdTree.h>
#include <pmp/algorithms/DifferentialGeometry.h>
#include <pmp/algorithms/distancePointTriangle.h>
#include <pmp/BoundingBox.h>
#include <cfloat>

//=============================================================================

namespace pmp {

//=============================================================================

TriangleKdTree::TriangleKdTree(const SurfaceMesh& mesh, unsigned int maxFaces,
                               unsigned int maxDepth)
{
    // init
    m_root = new Node();
    m_root->faces = new Triangles();
    SurfaceMesh::VertexProperty<Point> points =
        mesh.getVertexProperty<Point>("v:point");

    // collect triangles
    Triangle tri;
    m_root->faces->reserve(mesh.nFaces());
    for (SurfaceMesh::FaceIterator fit = mesh.facesBegin();
         fit != mesh.facesEnd(); ++fit)
    {
        SurfaceMesh::VertexAroundFaceCirculator vfit = mesh.vertices(*fit);
        tri.x[0] = points[*vfit];
        ++vfit;
        tri.x[1] = points[*vfit];
        ++vfit;
        tri.x[2] = points[*vfit];
        tri.f = *fit;
        m_root->faces->push_back(tri);
    }

    // call recursive helper
    buildRecurse(m_root, maxFaces, maxDepth);
}

//-----------------------------------------------------------------------------

unsigned int TriangleKdTree::buildRecurse(Node* node, unsigned int maxFaces,
                                          unsigned int depth)
{
    // should we stop at this level ?
    if ((depth == 0) || (node->faces->size() <= maxFaces))
        return depth;

    std::vector<Triangle>::const_iterator fit, fend = node->faces->end();
    unsigned int i;

    // compute bounding box
    BoundingBox bbox;
    for (fit = node->faces->begin(); fit != fend; ++fit)
    {
        for (i = 0; i < 3; ++i)
        {
            bbox += fit->x[i];
        }
    }

    // split longest side of bounding box
    Point bb = bbox.max() - bbox.min();
    Scalar length = bb[0];
    int axis = 0;
    if (bb[1] > length)
        length = bb[(axis = 1)];
    if (bb[2] > length)
        length = bb[(axis = 2)];

#if 1
    // split in the middle
    Scalar split = bbox.center()[axis];
#else
    // find split position as median
    std::vector<Scalar> v;
    v.reserve(node->faces->size() * 3);
    for (fit = node->faces->begin(); fit != fend; ++fit)
        for (i = 0; i < 3; ++i)
            v.push_back(fit->x[i][axis]]);
    std::sort(v.begin(), v.end());
    split = v[v.size() / 2];
#endif

    // create children
    auto* left = new Node();
    left->faces = new Triangles();
    left->faces->reserve(node->faces->size() / 2);
    auto* right = new Node();
    right->faces = new Triangles;
    right->faces->reserve(node->faces->size() / 2);

    // partition for left and right child
    for (fit = node->faces->begin(); fit != fend; ++fit)
    {
        bool l = false, r = false;

        const Triangle& t = *fit;
        if (t.x[0][axis] <= split)
            l = true;
        else
            r = true;
        if (t.x[1][axis] <= split)
            l = true;
        else
            r = true;
        if (t.x[2][axis] <= split)
            l = true;
        else
            r = true;

        if (l)
        {
            left->faces->push_back(t);
        }

        if (r)
        {
            right->faces->push_back(t);
        }
    }

    // stop here?
    if (left->faces->size() == node->faces->size() ||
        right->faces->size() == node->faces->size())
    {
        // compact my memory
        node->faces->shrink_to_fit();

        // delete new nodes
        delete left;
        delete right;

        // return tree depth
        return depth;
    }

    // or recurse further?
    else
    {
        // free my memory
        delete node->faces;
        node->faces = nullptr;

        // store internal data
        node->axis = axis;
        node->split = split;
        node->leftChild = left;
        node->rightChild = right;

        // recurse to childen
        int depthLeft = buildRecurse(node->leftChild, maxFaces, depth - 1);
        int depthRight = buildRecurse(node->rightChild, maxFaces, depth - 1);

        return std::min(depthLeft, depthRight);
    }
}

//-----------------------------------------------------------------------------

TriangleKdTree::NearestNeighbor TriangleKdTree::nearest(const Point& p) const
{
    NearestNeighbor data;
    data.dist = FLT_MAX;
    data.tests = 0;
    nearestRecurse(m_root, p, data);
    return data;
}

//-----------------------------------------------------------------------------

void TriangleKdTree::nearestRecurse(Node* node, const Point& point,
                                    NearestNeighbor& data) const
{
    // terminal node?
    if (!node->leftChild)
    {
        Scalar d;
        Point n;

        auto fit = node->faces->begin(), fend = node->faces->end();
        for (; fit != fend; ++fit)
        {
            d = distPointTriangle(point, fit->x[0], fit->x[1], fit->x[2], n);
            ++data.tests;
            if (d < data.dist)
            {
                data.dist = d;
                data.face = fit->f;
                data.nearest = n;
            }
        }
    }

    // non-terminal node
    else
    {
        Scalar dist = point[node->axis] - node->split;

        if (dist <= 0.0)
        {
            nearestRecurse(node->leftChild, point, data);
            if (fabs(dist) < data.dist)
                nearestRecurse(node->rightChild, point, data);
        }
        else
        {
            nearestRecurse(node->rightChild, point, data);
            if (fabs(dist) < data.dist)
                nearestRecurse(node->leftChild, point, data);
        }
    }
}

//=============================================================================
} // namespace pmp
//=============================================================================
