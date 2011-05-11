/**
 * @author Cyrille Collette - ccollette@aldebaran-robotics.com
 * Aldebaran Robotics (c) 2009 All Rights Reserved
 *
 */
#include <almath/types/alposition6d.h>
#include "../almathtestutils.h"

AL::Math::Position6D pPos6D1 = AL::Math::Position6D();
AL::Math::Position6D pPos6D2 = AL::Math::Position6D();

TEST(ALPosition6DTest, Distance)
{
  //std::cout << "-------------- Distance 0 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D();
  pPos6D2 = AL::Math::Position6D();
  EXPECT_NEAR(AL::Math::distance(pPos6D1, pPos6D2), 0.0f, kEpsilon);


  //std::cout << "-------------- Distance 1 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D();
  pPos6D2 = AL::Math::Position6D(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_NEAR(AL::Math::distance(pPos6D1, pPos6D2), 1.0f, kEpsilon);


  ////std::cout << "-------------- Distance 2 --------------" << std::endl;
  //pPos6D1 = AL::Math::Position6D(0.5f, -0.8f, 1.0f, +0.4f, -0.7f, -0.6f);
  //pPos6D2 = AL::Math::Position6D(1.0f, 1.2f, 1.2f, 1.0f, 1.2f, 1.2f);
  //EXPECT_NEAR(AL::Math::distance(pPos6D1, pPos6D2), 0.0f, kEpsilon); // TODO
}


TEST(ALPosition6DTest, norm)
{
  //std::cout << "-------------- norm 0 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D();
  EXPECT_NEAR(AL::Math::norm(pPos6D1), 0.0f, kEpsilon);


  //std::cout << "-------------- norm 1 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_NEAR(AL::Math::norm(pPos6D1), 1.0f, kEpsilon);


  ////std::cout << "-------------- norm 2 --------------" << std::endl;
  //pPos6D1 = AL::Math::Position6D(1.0f, -0.8f, 0.2f, 0.4f, 0.3f, -0.3f);
  //EXPECT_NEAR(AL::Math::norm(pPos6D1), 0.0f, kEpsilon); // TODO
}


TEST(ALPosition6DTest, normalize)
{
  //std::cout << "-------------- normalize 0 --------------" << std::endl;
  //pPos6D1 = AL::Math::Position6D(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  //ASSERT_THROW(AL::Math::normalize(pPos6D1), AL::ALError);


  //std::cout << "-------------- normalize 1 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  comparePosition6D(AL::Math::normalize(pPos6D1), AL::Math::Position6D(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));


  //std::cout << "-------------- normalize 2 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  comparePosition6D(AL::Math::normalize(pPos6D1), AL::Math::Position6D(0.70710678118655f, -0.70710678118655f, 0.0f, 0.0f, 0.0f, 0.0f));
}


TEST(ALPosition6DTest, Divers)
{
  //std::cout << "-------------- soustraction 0 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(+1.2f, 1.3f, 0.2f, +0.3f, 10.0f, -10.0f);
  pPos6D2 = AL::Math::Position6D(-0.5f, 0.2f, 0.4f, -0.5f,  0.2f,  +0.4f);
  comparePosition6D((pPos6D2-pPos6D1), AL::Math::Position6D(-1.7f, -1.1f, 0.2f, -0.8f, -9.8f, +10.4f));


  //std::cout << "-------------- addition 0 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(+1.2f, 1.3f, 0.3f, +1.2f, 1.3f, 0.3f);
  pPos6D2 = AL::Math::Position6D(-0.5f, 0.2f, 0.2f, -0.5f, 0.2f, 0.2f);
  comparePosition6D((pPos6D1+pPos6D2), AL::Math::Position6D(0.7f, 1.5f, 0.5f, 0.7f, 1.5f, 0.5f));


  //std::cout << "-------------- multiplication 0 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
  comparePosition6D((pPos6D1*2.0f), AL::Math::Position6D(2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f));


  ////std::cout << "-------------- multiplication 1 --------------" << std::endl;
  //pPos6D1 = AL::Math::Position6D(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
  //comparePosition6D((2.0f*pPos6D1), AL::Math::Position6D(2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f));


  //std::cout << "-------------- division 0 --------------" << std::endl;
  pPos6D1 = AL::Math::Position6D(2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f);
  comparePosition6D((pPos6D1/2.0f), AL::Math::Position6D(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f));


  //std::cout << "-------------- division 1 --------------" << std::endl;
  //pPos6D1 = AL::Math::Position6D(2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f);
  //ASSERT_THROW((pPos6D1/0.0f), AL::ALError);
}
