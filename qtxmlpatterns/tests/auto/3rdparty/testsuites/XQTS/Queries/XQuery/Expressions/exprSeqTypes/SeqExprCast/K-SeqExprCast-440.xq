(:*******************************************************:)
(: Test: K-SeqExprCast-440                               :)
(: Written by: Frans Englich                             :)
(: Date: 2007-11-22T11:31:21+01:00                       :)
(: Purpose: A simple test exercising the whitespace facet for type xs:float. :)
(:*******************************************************:)
xs:float("
	 3.4e5
	 ")
        eq
        xs:float("
	 3.4e5
	 ")
      