/*
MODIFIED BY AXEL ALMET: 23/12/14
Copyright (c) 2005-2014, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "EpithelialLayerLinearSpringForce.hpp"
#include "IsNan.hpp"
#include "AbstractCellProperty.hpp"

#include "HardCellMutationState.hpp"

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::EpithelialLayerLinearSpringForce()
   : AbstractTwoBodyInteractionForce<ELEMENT_DIM,SPACE_DIM>(),
     mEpithelialEpithelialSpringStiffness(15.0),
     mEpithelialNonepithelialSpringStiffness(15.0),
     mNonepithelialNonepithelialSpringStiffness(15.0),
     mHardCellStiffnessRatio(1.0)
{
    if (SPACE_DIM == 1)
    {
        mEpithelialEpithelialSpringStiffness = 30.0;
        mEpithelialNonepithelialSpringStiffness = 30.0;
        mNonepithelialNonepithelialSpringStiffness = 30.0;
    }
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::VariableSpringConstantMultiplicationFactor(unsigned nodeAGlobalIndex,
                                                                                     unsigned nodeBGlobalIndex,
                                                                                     AbstractCellPopulation<ELEMENT_DIM,SPACE_DIM>& rCellPopulation,
                                                                                     bool isCloserThanRestLength)
{
    return 1.0;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::~EpithelialLayerLinearSpringForce()
{
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
c_vector<double, SPACE_DIM> EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::CalculateForceBetweenNodes(unsigned nodeAGlobalIndex,
                                                                                    unsigned nodeBGlobalIndex,
                                                                                    AbstractCellPopulation<ELEMENT_DIM,SPACE_DIM>& rCellPopulation)
{
    // We should only ever calculate the force between two distinct nodes
    assert(nodeAGlobalIndex != nodeBGlobalIndex);

    Node<SPACE_DIM>* p_node_a = rCellPopulation.GetNode(nodeAGlobalIndex);
    Node<SPACE_DIM>* p_node_b = rCellPopulation.GetNode(nodeBGlobalIndex);

    // Get the node locations
    c_vector<double, SPACE_DIM> node_a_location = p_node_a->rGetLocation();
    c_vector<double, SPACE_DIM> node_b_location = p_node_b->rGetLocation();

    // Get the node radii for a NodeBasedCellPopulation
    double node_a_radius = 0.0;
    double node_b_radius = 0.0;

    if (dynamic_cast<NodeBasedCellPopulation<SPACE_DIM>*>(&rCellPopulation))
    {
        node_a_radius = p_node_a->GetRadius();
        node_b_radius = p_node_b->GetRadius();
    }

    // Get the unit vector parallel to the line joining the two nodes
    c_vector<double, SPACE_DIM> unit_difference;
    /*
     * We use the mesh method GetVectorFromAtoB() to compute the direction of the
     * unit vector along the line joining the two nodes, rather than simply subtract
     * their positions, because this method can be overloaded (e.g. to enforce a
     * periodic boundary in Cylindrical2dMesh).
     */
    unit_difference = rCellPopulation.rGetMesh().GetVectorFromAtoB(node_a_location, node_b_location);

    // Calculate the distance between the two nodes
    double distance_between_nodes = norm_2(unit_difference);
    assert(distance_between_nodes > 0);
    assert(!std::isnan(distance_between_nodes));

    unit_difference /= distance_between_nodes;

    /*
     * If mUseCutOffLength has been set, then there is zero force between
     * two nodes located a distance apart greater than mMechanicsCutOffLength in AbstractTwoBodyInteractionForce.
     */
    if (this->mUseCutOffLength)
    {
        if (distance_between_nodes >= this->GetCutOffLength())
        {
            return zero_vector<double>(SPACE_DIM); // c_vector<double,SPACE_DIM>() is not guaranteed to be fresh memory
        }
    }

    /*
     * Calculate the rest length of the spring connecting the two nodes with a default
     * value of 1.0.
     */
    double rest_length_final = 1.0;

    if (dynamic_cast<MeshBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>*>(&rCellPopulation))
    {
        rest_length_final = static_cast<MeshBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>*>(&rCellPopulation)->GetRestLength(nodeAGlobalIndex, nodeBGlobalIndex);
    }
    else if (dynamic_cast<NodeBasedCellPopulation<SPACE_DIM>*>(&rCellPopulation))
    {
        assert(node_a_radius > 0 && node_b_radius > 0);
        rest_length_final = node_a_radius+node_b_radius;
    }

    double rest_length = rest_length_final;

    CellPtr p_cell_A = rCellPopulation.GetCellUsingLocationIndex(nodeAGlobalIndex);
    CellPtr p_cell_B = rCellPopulation.GetCellUsingLocationIndex(nodeBGlobalIndex);

    double ageA = p_cell_A->GetAge();
    double ageB = p_cell_B->GetAge();

    assert(!std::isnan(ageA));
    assert(!std::isnan(ageB));

    /*
     * If the cells are both newly divided, then the rest length of the spring
     * connecting them grows linearly with time, until 1 hour after division.
     */
    if (ageA < mMeinekeSpringGrowthDuration && ageB < mMeinekeSpringGrowthDuration)
    {
        AbstractCentreBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>* p_static_cast_cell_population = static_cast<AbstractCentreBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>*>(&rCellPopulation);

        std::pair<CellPtr,CellPtr> cell_pair = p_static_cast_cell_population->CreateCellPair(p_cell_A, p_cell_B);

        if (p_static_cast_cell_population->IsMarkedSpring(cell_pair))
        {
            // Spring rest length increases from a small value to the normal rest length over 1 hour
            double lambda = mMeinekeDivisionRestingSpringLength;
            rest_length = lambda + (rest_length_final - lambda) * ageA/mMeinekeSpringGrowthDuration;
        }
        if (ageA + SimulationTime::Instance()->GetTimeStep() >= mMeinekeSpringGrowthDuration)
        {
            // This spring is about to go out of scope
            p_static_cast_cell_population->UnmarkSpring(cell_pair);
        }
    }

    /*
     * For apoptosis, progressively reduce the radius of the cell
     */
    double a_rest_length = rest_length*0.5;
    double b_rest_length = a_rest_length;

    if (dynamic_cast<NodeBasedCellPopulation<SPACE_DIM>*>(&rCellPopulation))
    {
        assert(node_a_radius > 0 && node_b_radius > 0);
        a_rest_length = (node_a_radius/(node_a_radius+node_b_radius))*rest_length;
        b_rest_length = (node_b_radius/(node_a_radius+node_b_radius))*rest_length;
    }

    /*
     * If either of the cells has begun apoptosis, then the length of the spring
     * connecting them decreases linearly with time.
     */
    if (p_cell_A->HasApoptosisBegun())
    {
        double time_until_death_a = p_cell_A->GetTimeUntilDeath();
        a_rest_length = a_rest_length * time_until_death_a / p_cell_A->GetApoptosisTime();
    }
    if (p_cell_B->HasApoptosisBegun())
    {
    	double time_until_death_b = p_cell_B->GetTimeUntilDeath();
    	b_rest_length = b_rest_length * time_until_death_b / p_cell_B->GetApoptosisTime();
    }

    rest_length = a_rest_length + b_rest_length;
    //assert(rest_length <= 1.0+1e-12); ///\todo #1884 Magic number: would "<= 1.0" do?

    //Checks if A and B are proliferative or differentiated cells.
    bool typeA = p_cell_A->GetCellProliferativeType()->IsType<DifferentiatedCellProliferativeType>();
    bool typeB = p_cell_B->GetCellProliferativeType()->IsType<DifferentiatedCellProliferativeType>();
	double overlap = distance_between_nodes - rest_length;
	bool is_closer_than_rest_length = (overlap <= 0);
	double multiplication_factor = VariableSpringConstantMultiplicationFactor(nodeAGlobalIndex, nodeBGlobalIndex, rCellPopulation, is_closer_than_rest_length);

    //If both the nodes are cells
    if ((typeA == typeB) && (! typeA))
    {
    	//Declare spring stiffness
    	double epithelial_epithelial_spring_stiffness = mEpithelialEpithelialSpringStiffness;

    	//If one of the cells is a Hard cell
    	if( (p_cell_A->GetMutationState()->IsType<HardCellMutationState>())||(p_cell_B->GetMutationState()->IsType<HardCellMutationState>()) )
    	{
        	epithelial_epithelial_spring_stiffness *= mHardCellStiffnessRatio;
    	}

    	if (dynamic_cast<MeshBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>*>(&rCellPopulation))
    	{
    		return multiplication_factor * epithelial_epithelial_spring_stiffness * unit_difference * overlap;
    	}
    	else
    	{
    		// A reasonably stable simple force law
    		if (is_closer_than_rest_length) //overlap is negative
    		{
    			//log(x+1) is undefined for x<=-1
    			assert(overlap > -rest_length_final);
    			c_vector<double, SPACE_DIM> temp = multiplication_factor*epithelial_epithelial_spring_stiffness * unit_difference * rest_length_final* log(1.0 + overlap/rest_length_final);
    			return temp;
    		}
    		else
    		{
    			double alpha = 5.0;
    			c_vector<double, SPACE_DIM> temp = multiplication_factor*epithelial_epithelial_spring_stiffness * unit_difference * overlap * exp(-alpha * overlap/rest_length_final);
    			return temp;
    		}
    	}
    }
    //If both the nodes represent the Matrigel
    else if  ((typeA == typeB) && (typeA))
    {
    	double nonepithelial_nonepithelial_spring_stiffness = mNonepithelialNonepithelialSpringStiffness;

    	if (dynamic_cast<MeshBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>*>(&rCellPopulation))
    	{
    		return multiplication_factor * nonepithelial_nonepithelial_spring_stiffness * unit_difference * overlap;
    	}
    	else
    	{
    		// A reasonably stable simple force law
    		if (is_closer_than_rest_length) //overlap is negative
    		{
    			//log(x+1) is undefined for x<=-1
    			assert(overlap > -rest_length_final);
    			c_vector<double, SPACE_DIM> temp = multiplication_factor*nonepithelial_nonepithelial_spring_stiffness * unit_difference * rest_length_final* log(1.0 + overlap/rest_length_final);
    			return temp;
    		}
    		else
    		{
    			double alpha = 5.0;
    			c_vector<double, SPACE_DIM> temp = multiplication_factor*nonepithelial_nonepithelial_spring_stiffness * unit_difference * overlap * exp(-alpha * overlap/rest_length_final);
    			return temp;
    		}
    	}
    }
    //If we have a cell-Matrigel pair
    else
    {
    	//Declare spring stiffness
    	double epithelial_nonepithelial_spring_stiffness = mEpithelialNonepithelialSpringStiffness;

    	//If the cell is a Hard cell (either A or B)
    	if(p_cell_A->GetMutationState()->IsType<HardCellMutationState>())
    	{
    		epithelial_nonepithelial_spring_stiffness *= mHardCellStiffnessRatio;

    	}
    	else if (p_cell_B->GetMutationState()->IsType<HardCellMutationState>())
    	{
    		epithelial_nonepithelial_spring_stiffness *= mHardCellStiffnessRatio;
    	}

    	if (dynamic_cast<MeshBasedCellPopulation<ELEMENT_DIM,SPACE_DIM>*>(&rCellPopulation))
    	{
    		return multiplication_factor * epithelial_nonepithelial_spring_stiffness * unit_difference * overlap;
    	}
    	else
    	{
    		// A reasonably stable simple force law
    		if (is_closer_than_rest_length) //overlap is negative
    		{
    			//log(x+1) is undefined for x<=-1
    			assert(overlap > -rest_length_final);
    			c_vector<double, SPACE_DIM> temp = multiplication_factor*epithelial_nonepithelial_spring_stiffness * unit_difference * rest_length_final* log(1.0 + overlap/rest_length_final);
    			return temp;
    		}
    		else
    		{
    			double alpha = 5.0;
    			c_vector<double, SPACE_DIM> temp = multiplication_factor*epithelial_nonepithelial_spring_stiffness * unit_difference * overlap * exp(-alpha * overlap/rest_length_final);
    			return temp;
    		}
    	}
    }


}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::GetEpithelialEpithelialSpringStiffness()
{
    return mEpithelialEpithelialSpringStiffness;
}
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::GetEpithelialNonepithelialSpringStiffness()
{
    return mEpithelialNonepithelialSpringStiffness;
}
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::GetNonepithelialNonepithelialSpringStiffness()
{
    return mNonepithelialNonepithelialSpringStiffness;
}
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::GetMeinekeDivisionRestingSpringLength()
{
    return mMeinekeDivisionRestingSpringLength;
}
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::GetMeinekeSpringGrowthDuration()
{
    return mMeinekeSpringGrowthDuration;
}
template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
double EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::GetHardCellStiffnessRatio()
{
    return mHardCellStiffnessRatio;
}


template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::SetEpithelialEpithelialSpringStiffness(double epithelialEpithelialSpringStiffness)
{
    assert(epithelialEpithelialSpringStiffness > 0.0);
    mEpithelialEpithelialSpringStiffness = epithelialEpithelialSpringStiffness;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::SetEpithelialNonepithelialSpringStiffness(double epithelialNonepithelialSpringStiffness)
{
    assert(epithelialNonepithelialSpringStiffness > 0.0);
    mEpithelialNonepithelialSpringStiffness = epithelialNonepithelialSpringStiffness;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::SetNonepithelialNonepithelialSpringStiffness(double nonepithelialNonepithelialSpringStiffness)
{
    assert(nonepithelialNonepithelialSpringStiffness > 0.0);
    mNonepithelialNonepithelialSpringStiffness = nonepithelialNonepithelialSpringStiffness;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::SetMeinekeDivisionRestingSpringLength(double divisionRestingSpringLength)
{
    assert(divisionRestingSpringLength <= 1.0);
    assert(divisionRestingSpringLength >= 0.0);

    mMeinekeDivisionRestingSpringLength = divisionRestingSpringLength;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::SetMeinekeSpringGrowthDuration(double springGrowthDuration)
{
    assert(springGrowthDuration >= 0.0);

    mMeinekeSpringGrowthDuration = springGrowthDuration;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::SetHardCellStiffnessRatio(double HardCellStiffnessRatio)
{
    assert(HardCellStiffnessRatio >= 0.0);

    mHardCellStiffnessRatio = HardCellStiffnessRatio;
}

template<unsigned ELEMENT_DIM, unsigned SPACE_DIM>
void EpithelialLayerLinearSpringForce<ELEMENT_DIM,SPACE_DIM>::OutputForceParameters(out_stream& rParamsFile)
{
    *rParamsFile << "\t\t\t<EpithelialEpithelialSpringStiffness>" << mEpithelialEpithelialSpringStiffness << "</EpithelialEpithelialSpringStiffness>\n";
    *rParamsFile << "\t\t\t<EpithelialNonepithelialSpringStiffness>" << mEpithelialNonepithelialSpringStiffness << "</EpithelialNonepithelialSpringStiffness>\n";
    *rParamsFile << "\t\t\t<NonepithelialNonepithelialSpringStiffness>" << mNonepithelialNonepithelialSpringStiffness << "</NonepithelialNonepithelialSpringStiffness>\n";
    *rParamsFile << "\t\t\t<MeinekeDivisionRestingSpringLength>" << mMeinekeDivisionRestingSpringLength << "</MeinekeDivisionRestingSpringLength>\n";
    *rParamsFile << "\t\t\t<MeinekeSpringGrowthDuration>" << mMeinekeSpringGrowthDuration << "</MeinekeSpringGrowthDuration>\n";
    *rParamsFile << "\t\t\t<HardCellStiffnessRatio>" << mHardCellStiffnessRatio << "</HardCellStiffnessRatio>\n";

    // Call method on direct parent class
    AbstractTwoBodyInteractionForce<ELEMENT_DIM,SPACE_DIM>::OutputForceParameters(rParamsFile);
}

/////////////////////////////////////////////////////////////////////////////
// Explicit instantiation
/////////////////////////////////////////////////////////////////////////////

template class EpithelialLayerLinearSpringForce<1,1>;
template class EpithelialLayerLinearSpringForce<1,2>;
template class EpithelialLayerLinearSpringForce<2,2>;
template class EpithelialLayerLinearSpringForce<1,3>;
template class EpithelialLayerLinearSpringForce<2,3>;
template class EpithelialLayerLinearSpringForce<3,3>;

// Serialization for Boost >= 1.36
#include "SerializationExportWrapperForCpp.hpp"
EXPORT_TEMPLATE_CLASS_ALL_DIMS(EpithelialLayerLinearSpringForce)
