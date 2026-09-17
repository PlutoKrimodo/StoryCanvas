function Home() {
  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-purple-50">
      <div className="container mx-auto px-4 py-16">
        <h1 className="text-4xl font-bold text-center text-gray-800 mb-8">
          StoryCanvas
        </h1>
        <p className="text-xl text-center text-gray-600 mb-12">
          基于生成式 AI 的儿童绘本智能创作与在线编辑系统
        </p>
        <div className="flex justify-center gap-6">
          <button className="px-8 py-3 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors font-medium">
            开始创作
          </button>
          <button className="px-8 py-3 bg-white text-blue-600 border-2 border-blue-600 rounded-lg hover:bg-blue-50 transition-colors font-medium">
            浏览绘本
          </button>
        </div>
      </div>
    </div>
  )
}

export default Home
