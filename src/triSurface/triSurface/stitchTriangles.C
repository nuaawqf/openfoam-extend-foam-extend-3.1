/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "triSurface.H"
#include "mergePoints.H"
#include "PackedBoolList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool triSurface::stitchTriangles
(
    const pointField& rawPoints,
    const scalar tol,
    bool verbose
)
{
    // Merge points
    labelList pointMap;
    pointField newPoints;
    bool hasMerged = mergePoints(rawPoints, tol, verbose, pointMap, newPoints);

    pointField& ps = storedPoints();

    // Set the coordinates to the merged ones
    ps.transfer(newPoints);

    if (hasMerged)
    {
        if (verbose)
        {
            Pout<< "stitchTriangles : Merged from " << rawPoints.size()
                << " points down to " << ps.size() << endl;
        }

        // Reset the triangle point labels to the unique points array
        label newTriangleI = 0;
        forAll(*this, i)
        {
            const labelledTri& tri = operator[](i);
            labelledTri newTri
            (
                pointMap[tri[0]],
                pointMap[tri[1]],
                pointMap[tri[2]],
                tri.region()
            );

            if ((newTri[0] != newTri[1]) && (newTri[0] != newTri[2]) && (newTri[1] != newTri[2]))
            {
                operator[](newTriangleI++) = newTri;
            }
            else if (verbose)
            {
                Pout<< "stitchTriangles : "
                    << "Removing triangle " << i
                    << " with non-unique vertices." << endl
                    << "    vertices   :" << newTri << endl
                    << "    coordinates:" << newTri.points(ps)
                    << endl;
            }
        }

        if (newTriangleI != size())
        {
            if (verbose)
            {
                Pout<< "stitchTriangles : "
                    << "Removed " << size() - newTriangleI
                    << " triangles" << endl;
            }
            setSize(newTriangleI);

            // And possibly compact out any unused points (since used only
            // by triangles that have just been deleted)
            // Done in two passes to save memory (pointField)

            // 1. Detect only
            PackedBoolList pointIsUsed(ps.size());

            label nPoints = 0;

            forAll(*this, i)
            {
                const labelledTri& tri = operator[](i);

                forAll(tri, fp)
                {
                    label pointI = tri[fp];
                    if (pointIsUsed.set(pointI, 1))
                    {
                        nPoints++;
                    }
                }
            }

            if (nPoints != ps.size())
            {
                // 2. Compact.
                pointMap.setSize(ps.size());
                label newPointI = 0;
                forAll(pointIsUsed, pointI)
                {
                    if (pointIsUsed[pointI])
                    {
                        ps[newPointI] = ps[pointI];
                        pointMap[pointI] = newPointI++;
                    }
                }
                ps.setSize(newPointI);

                newTriangleI = 0;
                forAll(*this, i)
                {
                    const labelledTri& tri = operator[](i);
                    operator[](newTriangleI++) = labelledTri
                    (
                        pointMap[tri[0]],
                        pointMap[tri[1]],
                        pointMap[tri[2]],
                        tri.region()
                    );
                }
            }
        }
    }

    return hasMerged;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
