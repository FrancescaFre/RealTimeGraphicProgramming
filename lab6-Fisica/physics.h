#pragma once

#include <bullet/btBulletDynamicsCommon.h>
enum shapes{BOX, SPHERE};

class Physics {
public:
  //global attribute
    btDiscreteDynamicsWorld* dynamicsWorld;
    btAlignedObjectArray<btCollisionShape*> collisionShapes; //vettore
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher; //collision manager
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;

    //costruttore
    Physics(){
      this->collisionConfiguration = new btDefaultCollisionConfiguration();
      this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);
      this->overlappingPairCache = new btDbvtBroadphase();

      //solver dell'equazion fisica per applicare rotazione e traslazione dell'oggetto
      this->solver = new btSequentialImpulseConstraintSolver();

      this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher, this->overlappingPairCache, this->solver, this->collisionConfiguration);

      //setto la gravità
      this->dynamicsWorld->setGravity(btVector3(0.0f, -9.82f, 0.0f)); //si usa la freccia perchè dynamicsWorld è un puntatore ad una classe
    }

    btRigidBody *createRigidBody(int type, glm::vec3 pos, glm::vec3 size, glm::vec3 rot, float m, float friction, float restitution){ //m =mass, friction=attrito applicato, restitution=quando qualcosa colpice l'oggetto, quante forza si perde nella collisionShapes
    {
      btCollisionShape* cShape = NULL;
      btVector3 position = btVector3(pos.x, pos.y, pos.z);
      btQuaternion rotation;
      rotation.setEuler(rot.x, rot.y, rot.z);

      if(type == BOX)
      {
        btVector3 dim = btVector3(size.x, size.y, size.y);
        cShape = new btBoxShape(dim); //box collision shape di questa dimensione

      }
      else if(type == SPHERE)
        cShape = new btSphereShape(size.x); //viene considerato come il raggio


      this->collisionShapes.push_back(cShape);

      btTransform objTransform;
      objTransform.setIdentity();
      objTransform.setRotation(rotation);
      objTransform.setOrigin(position);

      btScalar mass = m;
      //la massa del piano deve essere 0, perchè non deve essere soggeto alla gravità, è un oggetto statico
      bool isDynamic = (mass != 0.0f);
      btVector3 localInertia(0.0f, 0.0f, 0.0f);
      if (isDynamic)
        cShape->calculateLocalInertia(mass, localInertia);

      btDefaultMotionState* motionState = new btDefaultMotionState(objTransform);

      btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, cShape, localInertia); //qui si crea il rigidbody con il cShape appena creation

      rbInfo.m_friction = friction;
      rbInfo.m_restitution = restitution;

      if(type == SPHERE) //l sfera tocca in un punto solo il piano, qundi la friction non basta
      {
        rbInfo.m_angularDamping = 0.3; //resistenza allla rotazione
        rbInfo.m_rollingFriction = 0.3;  //frizione applicata ad una sfera che rotola su una superficie/
      }

      btRigidBody* body = new btRigidBody(rbInfo);
      this->dynamicsWorld->addRigidBody(body); //ora il rigidbody è aggiunto alla mainphysics class
      return body;
    }

    void Clear()
    {
      for (int i = this->dynamicsWorld->getNumCollisionObjects()-1; i>=0; i--)
      {
        btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectsArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj); //upcast btCollisionObject estende btRigidBody, è tipo un super perchè converte alla classe del padre
        if(body && body->getMotionState())
          delete body->getMotionState();
        this->dynamicsWorld->removeCollisionObject(obj);
        delete obj;

        for(int j = 0; j<this->collisionShapes.size(); j++)
        {
          btCollisionShape* shape = this->collisionShapes[j];
          this->collisionShapes[j]=0;
          delete shape;
        }
      }

      delete this->dynamicsWorld;
      delete this->solver;
      delete this->overlappingPairCache;
      delete this->dispatcher;
      delete this->collisionConfiguration;
      this->collisionShapes.clear();


    }

};
