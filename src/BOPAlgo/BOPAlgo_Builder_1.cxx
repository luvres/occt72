// Created by: Peter KURNEV
// Copyright (c) 2010-2014 OPEN CASCADE SAS
// Copyright (c) 2007-2010 CEA/DEN, EDF R&D, OPEN CASCADE
// Copyright (c) 2003-2007 OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN, CEDRAT,
//                         EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.


#include <BOPAlgo_Builder.hxx>
#include <BOPAlgo_PaveFiller.hxx>
#include <BOPCol_DataMapOfIntegerInteger.hxx>
#include <BOPCol_ListOfShape.hxx>
#include <BOPCol_MapOfShape.hxx>
#include <BOPDS_DS.hxx>
#include <BOPDS_ListOfPaveBlock.hxx>
#include <BOPDS_PaveBlock.hxx>
#include <BOPDS_ShapeInfo.hxx>
#include <BOPDS_VectorOfListOfPaveBlock.hxx>
#include <BOPTools_AlgoTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <IntTools_Context.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Shape.hxx>

//=======================================================================
//function : FillImagesVertices
//purpose  : 
//=======================================================================
void BOPAlgo_Builder::FillImagesVertices()
{
  Standard_Integer nV, nVSD;
  BOPCol_DataMapIteratorOfDataMapOfIntegerInteger aIt;
  //
  const BOPCol_DataMapOfIntegerInteger& aMSDV=myDS->ShapesSD();
  aIt.Initialize(aMSDV);
  for (; aIt.More(); aIt.Next()) {
    nV=aIt.Key();
    nVSD=aIt.Value();
    const TopoDS_Shape& aV=myDS->Shape(nV);
    const TopoDS_Shape& aVSD=myDS->Shape(nVSD);
    //
    BOPCol_ListOfShape aLVSD(myAllocator);
    //
    aLVSD.Append(aVSD);
    myImages.Bind(aV, aLVSD);
    //
    myShapesSD.Bind(aV, aVSD);
    //
    BOPCol_ListOfShape* pLOr = myOrigins.ChangeSeek(aVSD);
    if (!pLOr) {
      pLOr = myOrigins.Bound(aVSD, BOPCol_ListOfShape());
    }
    pLOr->Append(aV);
  }
}
//=======================================================================
//function : FillImagesEdges
//purpose  : 
//=======================================================================
  void BOPAlgo_Builder::FillImagesEdges()
{
  Standard_Integer i, aNbS = myDS->NbSourceShapes();
  for (i = 0; i < aNbS; ++i) {
    const BOPDS_ShapeInfo& aSI = myDS->ShapeInfo(i);
    if (aSI.ShapeType() != TopAbs_EDGE) {
      continue;
    }
    //
    // Check if the pave blocks for the edge have been initialized
    if (!aSI.HasReference()) {
      continue;
    }
    //
    const TopoDS_Shape& aE = aSI.Shape();
    const BOPDS_ListOfPaveBlock& aLPB = myDS->PaveBlocks(i);
    //
    // Fill the images of the edge from the list of its pave blocks.
    // The small edges, having no pave blocks, will have the empty list
    // of images and, thus, will be avoided in the result.
    BOPCol_ListOfShape *pLS = myImages.Bound(aE, BOPCol_ListOfShape());
    //
    BOPDS_ListIteratorOfListOfPaveBlock aItPB(aLPB);
    for (; aItPB.More(); aItPB.Next()) {
      const Handle(BOPDS_PaveBlock)& aPB = aItPB.Value();
      Handle(BOPDS_PaveBlock) aPBR = myDS->RealPaveBlock(aPB);
      //
      Standard_Integer nSpR = aPBR->Edge();
      const TopoDS_Shape& aSpR = myDS->Shape(nSpR);
      pLS->Append(aSpR);
      //
      BOPCol_ListOfShape* pLOr = myOrigins.ChangeSeek(aSpR);
      if (!pLOr) {
        pLOr = myOrigins.Bound(aSpR, BOPCol_ListOfShape());
      }
      pLOr->Append(aE);
      //
      if (myDS->IsCommonBlockOnEdge(aPB)) {
        Standard_Integer nSp = aPB->Edge();
        const TopoDS_Shape& aSp = myDS->Shape(nSp);
        myShapesSD.Bind(aSp, aSpR);
      }
    }
  }
}
//=======================================================================
// function: IsInterferred
// purpose: 
//=======================================================================
  Standard_Boolean BOPAlgo_Builder::IsInterferred(const TopoDS_Shape& theS)const
{
  Standard_Boolean bInterferred;
  TopoDS_Iterator aIt;
  //
  bInterferred=Standard_False;
  aIt.Initialize(theS);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aSx=aIt.Value();
    if (myImages.IsBound(aSx)) {
      bInterferred=!bInterferred;
      break;
    }
  }
  return bInterferred;
}
//=======================================================================
//function : BuildResult
//purpose  : 
//=======================================================================
  void BOPAlgo_Builder::BuildResult(const TopAbs_ShapeEnum theType)
{
  TopAbs_ShapeEnum aType;
  BRep_Builder aBB;
  BOPCol_MapOfShape aM;
  BOPCol_ListIteratorOfListOfShape aIt, aItIm;
  //
  aIt.Initialize(myArguments);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aS=aIt.Value();
    aType=aS.ShapeType();
    if (aType==theType) {
      if (myImages.IsBound(aS)){
        const BOPCol_ListOfShape& aLSIm=myImages.Find(aS);
        aItIm.Initialize(aLSIm);
        for (; aItIm.More(); aItIm.Next()) {
          const TopoDS_Shape& aSIm=aItIm.Value();
          if (aM.Add(aSIm)) {
            aBB.Add(myShape, aSIm);
          }
        }
      }
      else {
        if (aM.Add(aS)) {
          aBB.Add(myShape, aS);
        }
      }
    }
  }
}
//=======================================================================
// function: FillImagesContainers
// purpose: 
//=======================================================================
  void BOPAlgo_Builder::FillImagesContainers(const TopAbs_ShapeEnum theType)
{
  Standard_Integer i, aNbS;
  BOPCol_MapOfShape aMFP(100, myAllocator);
  //
  aNbS=myDS->NbSourceShapes();
  for (i=0; i<aNbS; ++i) {
    const BOPDS_ShapeInfo& aSI=myDS->ShapeInfo(i);
    if (aSI.ShapeType()==theType) {
      const TopoDS_Shape& aC=aSI.Shape();
      FillImagesContainer(aC, theType);
    }   
  }// for (; aItS.More(); aItS.Next()) {
}
//=======================================================================
// function: FillImagesCompounds
// purpose: 
//=======================================================================
  void BOPAlgo_Builder::FillImagesCompounds()
{
  Standard_Integer i, aNbS;
  BOPCol_MapOfShape aMFP(100, myAllocator);
  //
  aNbS=myDS->NbSourceShapes();
  for (i=0; i<aNbS; ++i) {
    const BOPDS_ShapeInfo& aSI=myDS->ShapeInfo(i);
    if (aSI.ShapeType()==TopAbs_COMPOUND) {
      const TopoDS_Shape& aC=aSI.Shape();
      FillImagesCompound(aC, aMFP);
    }
  }// for (; aItS.More(); aItS.Next()) {
}
//=======================================================================
//function : FillImagesContainer
//purpose  : 
//=======================================================================
  void BOPAlgo_Builder::FillImagesContainer(const TopoDS_Shape& theS,
                                            const TopAbs_ShapeEnum theType)
{
  Standard_Boolean bInterferred, bToReverse;
  TopoDS_Iterator aIt;
  BRep_Builder aBB;
  BOPCol_ListIteratorOfListOfShape aItIm; 
  //
  bInterferred=IsInterferred(theS);
  if (!bInterferred){
    return;
  }
  //
  TopoDS_Shape aCIm;
  BOPTools_AlgoTools::MakeContainer(theType, aCIm);
  //
  aIt.Initialize(theS);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aSx=aIt.Value();
    if (myImages.IsBound(aSx)) {
      const BOPCol_ListOfShape& aLFIm=myImages.Find(aSx);
      aItIm.Initialize(aLFIm);
      for (; aItIm.More(); aItIm.Next()) {
        TopoDS_Shape aSxIm=aItIm.Value();
        //
        bToReverse=BOPTools_AlgoTools::IsSplitToReverse(aSxIm, aSx, myContext);
        if (bToReverse) {
          aSxIm.Reverse();
        }
        aBB.Add(aCIm, aSxIm);
      }
    }
    else {
      aBB.Add(aCIm, aSx);
    }
  }
  //
  aCIm.Closed(BRep_Tool::IsClosed(aCIm));
  //
  BOPCol_ListOfShape aLSIm(myAllocator);
  aLSIm.Append(aCIm);
  myImages.Bind(theS, aLSIm); 
}
//=======================================================================
//function : FillImagesCompound
//purpose  : 
//=======================================================================
  void BOPAlgo_Builder::FillImagesCompound(const TopoDS_Shape& theS,
                                           BOPCol_MapOfShape& theMFP)
{ 
  Standard_Boolean bInterferred;
  TopAbs_Orientation aOrX;
  TopoDS_Iterator aIt;
  BRep_Builder aBB;
  BOPCol_ListIteratorOfListOfShape aItIm; 
  //
  if (!theMFP.Add(theS)) {
    return;
  }
  //
  bInterferred=Standard_False;
  aIt.Initialize(theS);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aSx=aIt.Value();
    if (aSx.ShapeType()==TopAbs_COMPOUND) {
      FillImagesCompound(aSx, theMFP);
    }
    if (myImages.IsBound(aSx)) {
      bInterferred=Standard_True;
    }
  }
  if (!bInterferred){
    return;
  }
  //
  TopoDS_Shape aCIm;
  BOPTools_AlgoTools::MakeContainer(TopAbs_COMPOUND, aCIm);
  //
  aIt.Initialize(theS);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aSX=aIt.Value();
    aOrX=aSX.Orientation();
    if (myImages.IsBound(aSX)) {
      const BOPCol_ListOfShape& aLFIm=myImages.Find(aSX);
      aItIm.Initialize(aLFIm);
      for (; aItIm.More(); aItIm.Next()) {
        TopoDS_Shape aSXIm=aItIm.Value();
        aSXIm.Orientation(aOrX);
        aBB.Add(aCIm, aSXIm);
      }
    }
    else {
      aBB.Add(aCIm, aSX);
    }
  }
  //
  BOPCol_ListOfShape aLSIm(myAllocator);
  aLSIm.Append(aCIm);
  myImages.Bind(theS, aLSIm); 
}
