
#include <memory>
#include <dune/common/function.hh>



template<class DT, class RT> class DifferentiableFunction;

template<class FImp> class FunctionHandle;



class InvalidRange
{};


template<class DT, class RT>
struct DerivativeTraits
{
    typedef InvalidRange DerivativeRangeType;
};

template<>
struct DerivativeTraits<double, double>
{
    typedef double DerivativeRangeType;
};



template<class DT, class RT>
class DifferentiableFunction :
    public Dune::VirtualFunction<DT, RT>,
    public std::enable_shared_from_this<DifferentiableFunction<DT, RT> >
{
    public:
        typedef DT DomainType;
        typedef RT RangeType;
        typedef typename DerivativeTraits<DT, RT>::Type DerivativeRangeType;

        typedef DifferentiableFunction<DomainType, DerivativeRangeType> DerivativeType;

    private:
        friend FunctionHandle<DerivativeType> derivative(const DifferentiableFunction&);

    protected:

        virtual DerivativeType* derivative() const = 0;

};


template<class FImp>
class FunctionHandle :
    public DifferentiableFunction<typename FImp::DomainType, typename FImp::RangeType>
{
        typedef DifferentiableFunction<typename FImp::DomainType, typename FImp::RangeType> Base;
    public:
        typedef typename Base::RangeType RangeType;
        typedef typename Base::DomainType DomainType;
        typedef typename Base::DerivativeRangeType DerivativeRangeType;

        typedef typename FImp::DerivativeType DerivativeType;

        // to be dicussed
        typedef FImp HandledFunctionType;
        typedef typename std::shared_ptr<const FImp> SharedPtr;

        explicit FunctionHandle(const FImp* f) :
            f_(f)
        {}


        virtual void evaluate(const DomainType& x, RangeType& y) const final
        {
            f_->evaluate(x, y);
        }

        std::shared_ptr<const FImp> shared_ptr() const
        {
            return f_->shared_from_this();
        }

    private:
        friend FunctionHandle<DerivativeType> derivative(const FunctionHandle<DerivativeType>&);

    protected:

        virtual DerivativeType* derivative() const final
        {
            return f_->derivative();
        }

        const FImp* f_;
};


template<class FImp>
FunctionHandle<typename FImp::DerivativeType> derivative(const FImp& f)
{
    return FunctionHandle<typename FImp::DerivativeType>(f.derivative());
}






template<class DT, class RT>
class InvalidFunction :
    Dune::VirtualFunction<DT, RT>
{
        typedef typename Dune::VirtualFunction<DT, RT> Base;
    public:
        typedef typename Base::RangeType RangeType;
        typedef typename Base::DomainType DomainType;
        typedef typename Base::DerivativeRangeType DerivativeRangeType;
        typedef InvalidFunction<DT, DerivativeRangeType> DerivativeType;

        virtual void evaluate(const DomainType& x, RangeType& y) const
        {
            throw 1;
        }

    protected:
        virtual DerivativeType* derivative() const
        {
            throw 1;
        }

};



/**
 * derivative(f).evaluate(x,y)
 * auto df = derivative(f);
 * auto dfp = derivative(f).shared_ptr();
 */