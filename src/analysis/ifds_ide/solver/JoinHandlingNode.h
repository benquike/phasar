/*
 * JoinHandlingNode.h
 *
 *  Created on: 23.11.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_JOINHANDLINGNODE_H_
#define ANALYSIS_IFDS_IDE_SOLVER_JOINHANDLINGNODE_H_

#include <vector>
using namespace std;

template <class T> class JoinHandlingNode {
public:
  virtual ~JoinHandlingNode();
  /**
   *
   * @param joiningNode the node abstraction that was propagated to the same
   * target after {@code this} node.
   * @return true if the join could be handled and no further propagation of the
   * {@code joiningNode} is necessary, otherwise false meaning
   * the node should be propagated by the solver.
   */
  virtual bool handleJoin(T joiningNode) = 0;

  class JoinKey {
  private:
    vector<T> elements;

  public:
    /**
     *
     * @param elements Passed elements must be immutable with respect to their
     * hashCode and equals implementations.
     */
    JoinKey(vector<T> elems) : elements(elems) {}
    int hash() { return 0; }
    bool equals() { return false; }
  };

  /**
   *
   * @return a JoinKey object used to identify which node abstractions require
   * manual join handling.
   * For nodes with {@code equal} JoinKey instances {@link
   * #handleJoin(JoinHandlingNode)} will be called.
   */
  virtual JoinKey createJoinKey() = 0;
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_JOINHANDLINGNODE_HH_ */
