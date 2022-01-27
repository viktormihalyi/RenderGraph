#include "gtest/gtest.h"
#include "RenderGraph/RenderGraphPass.hpp"

using RenderGraphPassTest = ::testing::Test;

TEST_F (RenderGraphPassTest, RenderGraphPassTest_SingleOutput)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);

    RG::Pass p;
    p.AddOutput (op1, res1);

    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (1, p.GetAllOutputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    EXPECT_EQ (res1, p.GetResourceIO (res1)->res);
    EXPECT_EQ (op1, p.GetOperationIO (op1)->op);
    EXPECT_EQ (1, p.GetOperationIO (op1)->outputs.size ());
    EXPECT_EQ (0, p.GetOperationIO (op1)->inputs.size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_SingleInput)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);

    RG::Pass p;
    p.AddInput (op1, res1);

    EXPECT_EQ (1, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    EXPECT_EQ (res1, p.GetResourceIO (res1)->res);
    EXPECT_EQ (op1, p.GetOperationIO (op1)->op);
    EXPECT_EQ (0, p.GetOperationIO (op1)->outputs.size ());
    EXPECT_EQ (1, p.GetOperationIO (op1)->inputs.size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_SingleOutput_Remove)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);

    RG::Pass p;
    p.AddOutput (op1, res1);
    p.RemoveOutput (op1, res1);

    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllOperations ().size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_SingleInput_Remove)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);

    RG::Pass p;
    p.AddInput (op1, res1);
    p.RemoveInput (op1, res1);

    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllOperations ().size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleInput)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);

    RG::Pass p;
    p.AddInput (op1, res1);
    p.AddInput (op1, res2);

    EXPECT_EQ (2, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleInput_RemoveOne)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);

    RG::Pass p;
    p.AddInput (op1, res1);
    p.AddInput (op1, res2);
    p.RemoveInput (op1, res2);

    EXPECT_EQ (1, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    EXPECT_EQ (res1, p.GetResourceIO (res1)->res);
    EXPECT_EQ (op1, p.GetOperationIO (op1)->op);
    EXPECT_EQ (0, p.GetOperationIO (op1)->outputs.size ());
    EXPECT_EQ (1, p.GetOperationIO (op1)->inputs.size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleInput_RemoveAll)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);
    RG::Resource*  res3 = reinterpret_cast<RG::Resource*> (4);

    RG::Pass p;
    p.AddInput (op1, res1);
    p.AddInput (op1, res2);
    p.AddInput (op1, res3);
    p.RemoveInput (op1, res2);
    p.RemoveInput (op1, res1);
    p.RemoveInput (op1, res3);

    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllOperations ().size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleOutput)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);

    RG::Pass p;
    p.AddOutput (op1, res1);
    p.AddOutput (op1, res2);

    EXPECT_EQ (2, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleOutput_RemoveOne)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);

    RG::Pass p;
    p.AddOutput (op1, res1);
    p.AddOutput (op1, res2);
    p.RemoveOutput (op1, res2);

    EXPECT_EQ (1, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    EXPECT_EQ (res1, p.GetResourceIO (res1)->res);
    EXPECT_EQ (op1, p.GetOperationIO (op1)->op);
    EXPECT_EQ (1, p.GetOperationIO (op1)->outputs.size ());
    EXPECT_EQ (0, p.GetOperationIO (op1)->inputs.size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleOutput_RemoveAll)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);
    RG::Resource*  res3 = reinterpret_cast<RG::Resource*> (4);

    RG::Pass p;
    p.AddOutput (op1, res1);
    p.AddOutput (op1, res2);
    p.AddOutput (op1, res3);
    p.RemoveOutput (op1, res2);
    p.RemoveOutput (op1, res1);
    p.RemoveOutput (op1, res3);

    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOperations ().size ());
}


TEST_F (RenderGraphPassTest, RenderGraphPassTest_MultipleIO)
{
    RG::Operation* op1  = reinterpret_cast<RG::Operation*> (1);
    RG::Resource*  res1 = reinterpret_cast<RG::Resource*> (2);
    RG::Resource*  res2 = reinterpret_cast<RG::Resource*> (3);
    RG::Resource*  res3 = reinterpret_cast<RG::Resource*> (4);

    RG::Pass p;
    p.AddInput (op1, res1);
    p.AddInput (op1, res2);
    p.AddOutput (op1, res3);

    EXPECT_EQ (1, p.GetAllOutputs ().size ());
    EXPECT_EQ (2, p.GetAllInputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    p.RemoveOutput (op1, res3);

    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (2, p.GetAllInputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    p.RemoveInput (op1, res2);

    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (1, p.GetAllInputs ().size ());
    EXPECT_EQ (1, p.GetAllOperations ().size ());

    p.RemoveInput (op1, res1);

    EXPECT_EQ (0, p.GetAllOutputs ().size ());
    EXPECT_EQ (0, p.GetAllInputs ().size ());
    EXPECT_EQ (0, p.GetAllOperations ().size ());
}
